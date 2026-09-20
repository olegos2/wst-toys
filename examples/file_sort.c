#include "toys/sort.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))


/** A utf8 line + decoded wchar repr */
typedef struct {
    /* UTF-8 encoded bytes ptr */
    const char *bptr;
    /* UTF-8 encoding length in bytes */
    size_t blen;
    /* wchar decoded str */
    wchar_t *wptr;
    /* wchar decoded str length (chars count) */
    size_t wlen;
} WstLine;


/* Test if line has any alphabetic characters */
static bool has_walpha(const WstLine *line)
{
    for (size_t i = 0; i < line->wlen; i++)
        if (iswalpha((wint_t)line->wptr[i]))
            return true;
    return false;
}


/**
 * Compare `WstLine` wchar strings lowered, skip non-alphabetic chars.
 */
static int cmp_letters_fwd(const void *a, const void *b)
{
    const WstLine *line_a = (const WstLine *)a;
    const WstLine *line_b = (const WstLine *)b;
    size_t i = 0, j = 0;

    while (1) {
        while (i < line_a->wlen && !iswalpha((wint_t)line_a->wptr[i]))
            i++;
        while (j < line_b->wlen && !iswalpha((wint_t)line_b->wptr[j]))
            j++;

        if (i >= line_a->wlen || j >= line_b->wlen)
            return (j >= line_b->wlen) - (i >= line_a->wlen);

        wchar_t lower_a = (wchar_t)towlower((wint_t)line_a->wptr[i]);
        wchar_t lower_b = (wchar_t)towlower((wint_t)line_b->wptr[j]);
        if (lower_a != lower_b)
            return (lower_a < lower_b) ? -1 : 1;

        i++;
        j++;
    }
}

/**
 * Compare `WstLine` wchar strings lowered right-to-left, skip non-alphabetic chars.
 */
static int cmp_letters_rev(const void *a, const void *b)
{
    const WstLine *line_a = (const WstLine *)a;
    const WstLine *line_b = (const WstLine *)b;
    size_t i = line_a->wlen, j = line_b->wlen;

    while (1) {
        while (i > 0 && !iswalpha((wint_t)line_a->wptr[i - 1]))
            i--;
        while (j > 0 && !iswalpha((wint_t)line_b->wptr[j - 1]))
            j--;

        if (i == 0 || j == 0)
            return (i == j) ? 0 : ((i == 0) ? -1 : 1);

        wchar_t lower_a = (wchar_t)towlower((wint_t)line_a->wptr[i - 1]);
        wchar_t lower_b = (wchar_t)towlower((wint_t)line_b->wptr[j - 1]);
        if (lower_a != lower_b)
            return (lower_a < lower_b) ? -1 : 1;

        i--;
        j--;
    }
}

/**
 * A file mapped to memory.
 */
typedef struct {
    const char *data;
    size_t size;
    /**
     * Whether memory is backed by mmap and not malloc'd buffer
     */
    bool is_mmap;
} FileContent;


/**
 * Unmap/free file content buffer
 */
static void free_file(FileContent *content)
{
    if (content == NULL || content->data == NULL)
        return;

    if (content->is_mmap)
        munmap((void *)content->data, content->size);
    else
        free((void *)content->data);

    content->data = NULL;
    content->size = 0;
}

/**
 * Tries to mmap entire file at `path` to mem, or alloc then read if mmap fails.
 * Does not use text mode.
 */
static bool read_file(const char *path, FileContent *out)
{
    assert(path != NULL);
    assert(out != NULL);

    free_file(out);

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        LOG_E("cannot open '%s': %s", path, strerror(errno));
        return false;
    }

    struct stat my_stat = { 0 };
    if (fstat(fd, &my_stat) != 0) {
        LOG_E("cannot stat '%s': %s", path, strerror(errno));
        close(fd);
        return false;
    }

    if (!S_ISREG(my_stat.st_mode)) {
        LOG_E("'%s' is not a regular file", path);
        close(fd);
        return false;
    }

    size_t fsize = (size_t)my_stat.st_size;
    if (fsize == 0) {
        LOG_D("'%s' is an empty file", path);
        close(fd);
        return true;
    }

    void *map = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map != MAP_FAILED) {
        LOG_D("'%s': %zu bytes via mmap", path, fsize);
        out->data = (const char *)map;
        out->size = fsize;
        out->is_mmap = true;
        close(fd);
        return true;
    }
    LOG_W("mmap of '%s' failed: %s, falling back to read",
          path, strerror(errno));

    char *buf = calloc(fsize, sizeof(char));
    if (buf == NULL) {
        LOG_E("calloc(%zu) failed: %s", fsize, strerror(errno));
        close(fd);
        return false;
    }

    /* `read` does not guarantee reading all bytes in one go */
    size_t read_so_far = 0;
    while (read_so_far < fsize) {
        ssize_t ret = read(fd, buf + read_so_far, fsize - read_so_far);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            LOG_E("read of '%s' failed: %s", path, strerror(errno));
            free(buf);
            close(fd);
            return false;
        }
        if (ret == 0)
            break; /* EOF */
        read_so_far += (size_t)ret;
    }
    close(fd);

    out->data = buf;
    out->size = read_so_far;
    out->is_mmap = false;
    return true;
}

/** 
 * Find a suitable UTF-8 locale and set it. Prefers current/default.
 */
static bool setup_locale(void)
{
    static const char *try_locales[] = {
        "", "C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8",
    };

    for (size_t i = 0; i < ARR_LEN(try_locales); i++) {
        if (setlocale(LC_CTYPE, try_locales[i]) == NULL)
            continue;

        LOG_I("ctype locale: '%s'", setlocale(LC_CTYPE, NULL));
        return true;
    }

    LOG_E("no UTF-8 locale available");
    return false;
}

/**
 * Decode multibyte (UTF-8) string of `len` bytes into wide chars buffer.
 */
static wchar_t *decode_line(const char *ptr, size_t len, size_t *out_len)
{
    assert(ptr != NULL);
    assert(out_len != NULL);

    wchar_t *wide = calloc((len + 1), sizeof(wchar_t));
    if (wide == NULL) {
        LOG_E("calloc(%zu, %zu): %s", len + 1, sizeof(wchar_t), strerror(errno));
        return NULL;
    }

    mbstate_t ps = { 0 }; /* shift state */
    size_t byte_idx = 0;
    size_t wide_idx = 0;

    while (byte_idx < len) {
        wchar_t wc = L'\0';

        size_t used = mbrtowc(&wc, ptr + byte_idx, len - byte_idx, &ps);
        if (used == (size_t)-1 || used == (size_t)-2) {
            LOG_E("input is not valid UTF-8: %s", strerror(errno));
            free(wide);
            return NULL;
        }

        if (used == 0) /* got \0, add it to output */
            used = 1;

        wide[wide_idx++] = wc;
        byte_idx += used;
    }

    *out_len = wide_idx;
    return wide;
}

/* Decode array of multibyte lines into wchar representations */
static bool decode_lines(WstLine *lines, size_t n)
{
    assert(n == 0 || lines != NULL);

    for (size_t i = 0; i < n; i++) {
        lines[i].wptr = decode_line(lines[i].bptr, lines[i].blen, &lines[i].wlen);
        if (lines[i].wptr == NULL)
            return false;
    }

    return true;
}

static void free_lines(WstLine *lines, size_t n)
{
    if (lines == NULL)
        return;

    for (size_t i = 0; i < n; i++)
        free(lines[i].wptr);
    free(lines);
}

/* Split input buffer into headers for each line. Uses '\n' as delim with optional '\r'. */
static WstLine *split_lines(const char *data, size_t size, size_t *out_len)
{
    assert(out_len != NULL);

    if (data == NULL || size == 0) {
        *out_len = 0;
        return NULL;
    }

    /* count lines on first pass */
    /* it is safe to byte-scan ascii symbols in UTF-8 */
    size_t count = 0;
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n')
            count++;
    }

    if (data[size - 1] != '\n')
        count++; /* trailing line */

    if (count == 0)
        return NULL;

    WstLine *lines = calloc(count, sizeof(*lines));
    if (lines == NULL) {
        LOG_E("calloc(%zu, %zu): %s", count, sizeof(*lines), strerror(errno));
        return NULL;
    }

    size_t str_idx = 0;
    size_t start = 0;

    for (size_t i = 0; i < size; i++) {
        if (data[i] != '\n')
            continue;

        size_t end = i;
        if (end > start && data[end - 1] == '\r')
            end--;

        lines[str_idx].bptr = data + start;
        lines[str_idx].blen = end - start;
        str_idx++;
        start = i + 1;
    }
    /* add trailing line */
    if (start < size) {
        lines[str_idx].bptr = data + start;
        lines[str_idx].blen = size - start;
        str_idx++;
    }

    *out_len = str_idx;
    return lines;
}

static bool write_lines(FILE *stream, const WstLine *lines, size_t nlines)
{
    assert(stream != NULL);

    for (size_t line_idx = 0; line_idx < nlines; line_idx++) {
        if (!has_walpha(&lines[line_idx]))
            continue;

        size_t ret = fwrite(lines[line_idx].bptr, 1, lines[line_idx].blen, stream);
    
        if (ret != lines[line_idx].blen) {
            LOG_E("fwrite: %s", strerror(errno));
            return false;
        }

        if (fputc('\n', stream) != '\n') {
            LOG_E("fputc: %s", strerror(errno));
            return false;
        }
    }
    return true;
}


int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s in_filename [out_filename]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *in_path = argv[1];
    const char *out_path = (argc >= 3) ? argv[2] : NULL;

    if (!setup_locale())
        return EXIT_FAILURE;

    FileContent content = { 0 };
    if (!read_file(in_path, &content))
        return EXIT_FAILURE;

    size_t nlines = 0;
    WstLine *lines = split_lines(content.data, content.size, &nlines);
    if (nlines > 0 && lines == NULL) {
        free_file(&content);
        return EXIT_FAILURE;
    }

    if (!decode_lines(lines, nlines)) {
        free_lines(lines, nlines);
        free_file(&content);
        return EXIT_FAILURE;
    }
    LOG_I("'%s': %zu bytes, %zu lines",
          in_path, content.size, nlines);

    FILE *out = (out_path != NULL) ? fopen(out_path, "w") : stdout;
    if (out == NULL) {
        LOG_E("cannot open '%s' for writing: %s", out_path, strerror(errno));
        free_lines(lines, nlines);
        free_file(&content);
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;

    static const char *separator = "/* ----------------------------- */\n";

    do {
        wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_fwd);
        if (!write_lines(out, lines, nlines) ||
            fwrite(separator, 1, strlen(separator), out) != strlen(separator))
        {
            LOG_E("failed to write forward sorted lines");
            ret = EXIT_FAILURE;
            break;
        }

        wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_rev);
        if (!write_lines(out, lines, nlines) ||
            fwrite(separator, 1, strlen(separator), out) != strlen(separator))
        {
            LOG_E("failed to write backward sorted lines");
            ret = EXIT_FAILURE;
            break;
        }

        if (content.size > 0 &&
            fwrite(content.data, 1, content.size, out) != content.size)
        {
            LOG_E("failed to write original input contents to output");
            ret = EXIT_FAILURE;
            break;
        }
    } while (0);

    if (out != stdout && fclose(out) != 0) {
        LOG_E("failed closing '%s': %s", out_path, strerror(errno));
    }

    free_lines(lines, nlines);
    free_file(&content);

    if (ret == EXIT_SUCCESS && out_path != NULL)
        LOG_I("wrote '%s'", out_path);

    return ret;
}
