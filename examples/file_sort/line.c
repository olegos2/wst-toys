#include "file_sort.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
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


/* Test if line has any alphabetic characters */
bool has_walpha(const WstLine *line)
{
    for (size_t i = 0; i < line->wlen; i++)
        if (iswalpha((wint_t)line->wptr[i]))
            return true;
    return false;
}


int cmp_letters_fwd(const void *a, const void *b)
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

int cmp_letters_rev(const void *a, const void *b)
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

bool decode_lines(WstLine *lines, size_t nlines)
{
    assert(nlines == 0 || lines != NULL);

    for (size_t i = 0; i < nlines; i++) {
        lines[i].wptr = decode_line(lines[i].bptr, lines[i].blen, &lines[i].wlen);
        if (lines[i].wptr == NULL)
            return false;
    }

    return true;
}

void free_lines(WstLine *lines, size_t nlines)
{
    if (lines == NULL)
        return;

    for (size_t i = 0; i < nlines; i++)
        free(lines[i].wptr);
    free(lines);
}

WstLine *split_lines(const char *data, size_t size, size_t *out_len)
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

bool write_alpha_lines(FILE *stream, const WstLine *lines, size_t nlines)
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

