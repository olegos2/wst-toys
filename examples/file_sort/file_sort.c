#include "file_sort.h"
#include "toys/argparse.h"
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


/**
 * Unmap/free file content buffer
 */
static void free_file(FileContent *content);

/**
 * Tries to mmap entire file at `path` to mem, or alloc then read if mmap fails.
 * Does not use text mode.
 */
static bool read_file(const char *path, FileContent *out);

/** 
 * Find a suitable UTF-8 locale and set it. Prefers current/default.
 */
static bool setup_locale(void);


/**
 * Sort previously read and decoded `lines` and write them to
 * previously opened file `out`.
 * Does not close file or free lines.
 *
 * @return `true` if everything was written, `false` on failure.
 */
static bool write_sorted_output(FILE *out, WstLine *lines, size_t nlines,
                                const FileContent *content)
{
    static const char *separator = "/* ----------------------------- */\n";

    wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_fwd);
    if (!write_alpha_lines(out, lines, nlines) ||
        fwrite(separator, 1, strlen(separator), out) != strlen(separator))
    {
        LOG_E("failed to write forward sorted lines");
        return false;
    }
    LOG_I("wrote forward sorted lines");

    wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_rev);
    if (!write_alpha_lines(out, lines, nlines) ||
        fwrite(separator, 1, strlen(separator), out) != strlen(separator))
    {
        LOG_E("failed to write backward sorted lines");
        return false;
    }
    LOG_I("wrote backward sorted lines");

    if (content->size > 0 &&
        fwrite(content->data, 1, content->size, out) != content->size)
    {
        LOG_E("failed to write original input contents to output");
        return false;
    }
    LOG_I("catenated input to output");
    return true;
}


/**
 * - Read input file by path into a buffer,
 * - split it into strings,
 * - decode into wide char strings,
 * - sort strings in multiple ways,
 * - catenate original file contents,
 * - write all 3 variants to one output file.
 */
static int run_file_sort(const char *in_path, const char *out_path)
{
    assert(in_path != NULL);

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
    LOG_D("opened '%s' for writing", out_path);

    int ret = EXIT_SUCCESS;

    if (!write_sorted_output(out, lines, nlines, &content))
        ret = EXIT_FAILURE;

    if (out != stdout && fclose(out) != 0) {
        LOG_E("failed closing '%s': %s", out_path, strerror(errno));
    }

    free_lines(lines, nlines);
    free_file(&content);

    if (ret == EXIT_SUCCESS && out_path != NULL)
        LOG_I("wrote '%s'", out_path);

    return ret;
}


int main(int argc, char **argv)
{
    bool help = false;
    bool verbose = false;
    const char *debug_filename = NULL;
    const char *in_path = NULL;
    /* NULL out_path maps to stdout */
    const char *out_path = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &help,
            .short_name = "-h",
            .long_name = "--help",
            .description = "print this help and exit",
        },
#ifdef WST_DEBUG
        {
            .type = ARG_SWITCH,
            .dest = &verbose,
            .short_name = "-v",
            .long_name = "--verbose",
            .description = "enable verbose logging messages",
        },
#endif
        {
            .type = ARG_STRING,
            .dest = &debug_filename,
            .short_name = "-l",
            .long_name = "--logfile",
            .description = "redirect log prints to a file path",
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &in_path,
            .long_name = "in_filename",
            .description = "input file path to read",
            .required = true,
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &out_path,
            .long_name = "out_filename",
            .description = "output file path to write",
        },
    };

    ArgParser parser = {
        .prog = argv[0],
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    if (!argparse_parse(&parser, argc, argv)) {
        fprintf(stderr, "%s, run '%s --help' for usage\n", parser.error, argv[0]);
        return EXIT_FAILURE;
    }

    if (help) {
        argparse_print_help(&parser);
        return EXIT_SUCCESS;
    }

    if (verbose)
        wst_log_set_max_prio(WST_LOG_VERBOSE);

    if (!wst_log_open(debug_filename))
        LOG_W("failed to open log file for writing");

    if (!setup_locale())
        return EXIT_FAILURE;

    return run_file_sort(in_path, out_path);
}



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

static bool setup_locale(void)
{
    static const char *try_locales[] = {
        "", "C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8",
    };

    for (size_t i = 0; i < ARR_LEN(try_locales); i++) {
        if (setlocale(LC_CTYPE, try_locales[i]) == NULL)
            continue;

        LOG_D("ctype locale: '%s'", setlocale(LC_CTYPE, NULL));
        return true;
    }

    LOG_E("no UTF-8 locale available");
    return false;
}
