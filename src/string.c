#include "toys/string.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


#define BUCKET_SORT_LEN ((1 << 8) + 1)

/* TODO: Optimize some mem ops to use wider types */

void wst_qsort(void *base, size_t count, size_t size,
               WstComparator compar)
{
    // TODO
}

/**
 * Sorts strings by char at position `pos` (using radix/bucket sort).
 *
 * Works by:
 * - counting occurencies of every possible byte in strings,
 * - creating index table from this count buffer,
 * - using a buffer to store sorted result,
 * - inserting strings (pointers) into this buffer using table from before,
 * - copying pointers back.
 *
 * Recurses for every string subset where all strings have same char at position `pos`
 * (except '\0') to sort subsets using char at `pos + 1`.
 *
 * `buf` is passed down to avoid allocating it on stack at every depth level.
 *
 * @param[inout] strings Array of strings where `[start, end)` needs to be sorted.
 * @param[inout] buf Buffer that has at least `end - start` length.
 * @param[in] start Starting index inside array.
 * @param[in] end Ending index non-inclusive.
 * @param[in] pos Character index to look for in every string.
 */
static void wst_str_sort_rec(const unsigned char *strings[], const unsigned char *buf[],
                             size_t start, size_t end, size_t pos)
{
    assert(strings != NULL);
    assert(buf != NULL);

    if (start >= end - 1)
        return;

    /* `count[i + 1]` corresponds to number of occurencies
     * of a char value `i` in strings at position `pos` */
    size_t count[BUCKET_SORT_LEN] = { 0 };

    for (size_t i = start; i < end; i++)
        count[strings[i][pos] + 1]++;

    /* Convert `count` into table of indices in result buffer */
    for (size_t i = 2; i < BUCKET_SORT_LEN; i++)
        count[i] += count[i - 1];

    for (size_t i = start; i < end; i++)
        buf[count[strings[i][pos]]++] = strings[i];

    for (size_t i = start; i < end; i++) {
        /* Revert indices back to beginning of every subset. */
        count[strings[i][pos]]--;

        strings[i] = buf[i - start];
    }

    /* count[0] stores number of strings that have `\0` at `pos` */
    /* recurse for every subset except these */
    for (size_t i = 1; i < BUCKET_SORT_LEN - 1; i++) {
        wst_str_sort_rec(strings, buf, start + count[i],
                     start + count[i + 1], pos + 1);
    }
}

size_t wst_strlen(const char *str)
{
    assert(str != NULL);

    size_t i = 0;
    while (str[i] != '\0') i++;

    return i;
}

size_t wst_strnlen(const char *str, size_t max_len)
{
    assert(str != NULL);

    size_t i = 0;
    while (str[i] != '\0' && i < max_len) i++;

    return i;
}

const char *wst_strnul(const char *str)
{
    assert(str != NULL);

    const char *cur = str;
    while (*cur != '\0') cur++;

    return cur;
}

char *wst_stpcpy(char *restrict dest, const char *restrict src)
{
    assert(dest != NULL);
    assert(src != NULL);

    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';

    return dest + i;
}

char *wst_stpncpy(char *restrict dest, const char *restrict src, size_t dsize)
{
    assert(dest != NULL);
    assert(src != NULL);

    size_t i;
    for (i = 0; src[i] != '\0' && i < dsize; i++)
        dest[i] = src[i];

    size_t ret = i;

    for (; i < dsize; i++)
        dest[i] = '\0';

    return dest + ret;
}

char *wst_strcpy(char *restrict dest, const char *restrict src)
{
    wst_stpcpy(dest, src);

    return dest;
}

char *wst_strncpy(char *restrict dest, const char *restrict src, size_t dsize)
{
    wst_stpncpy(dest, src, dsize);

    return dest;
}

char *wst_strdup(const char *str)
{
    assert(str != NULL);

    char *dest = calloc(wst_strlen(str) + 1, sizeof(char));
    if (dest == NULL)
        return NULL;

    return wst_strcpy(dest, str);
}

char *wst_strndup(const char *str, size_t len)
{
    assert(len == 0 || str != NULL);

    size_t dsize = wst_strnlen(str, len) + 1;

    char *dest = calloc(dsize, sizeof(char));
    if (dest == NULL)
        return NULL;

    return wst_strncpy(dest, str, dsize);
}

int wst_memcmp(const void *str1, const void *str2, size_t len)
{
    assert(len == 0 || str1 != NULL);
    assert(len == 0 || str2 != NULL);

    for (size_t i = 0; i < len; i++) {
        unsigned char c1 = ((unsigned char *)str1)[i];
        unsigned char c2 = ((unsigned char *)str2)[i];
        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;
    }

    return 0;
}

void *wst_memcpy(void *restrict dest, const void *restrict src, size_t len)
{
    assert(dest != NULL);
    assert(src != NULL);

    for (size_t i = 0; i < len; i++) {
        ((unsigned char *)dest)[i] = ((unsigned char *)src)[i];
    }

    return dest;
}

int wst_strcmp(const char *str1, const char *str2)
{
    assert(str1 != NULL);
    assert(str2 != NULL);

    size_t i = 0;
    while (1) {
        unsigned char c1 = (unsigned char)str1[i];
        unsigned char c2 = (unsigned char)str2[i];
        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;

        if (str1[i] == '\0' || str2[i] == '\0')
            break;

        i++;
    }

    return 0;
}

char *wst_strcat(char *restrict dest, const char *restrict src)
{
    wst_strcpy((char *)wst_strnul(dest), src);

    return dest;
}

void *wst_memchr(const void *str, int c, size_t len)
{
    assert(len == 0 || str != NULL);

    const unsigned char *s = (const unsigned char *)str;

    for (size_t i = 0; i < len; i++) {
        if (s[i] == (unsigned char)c)
            return (void *)(s + i);
    }

    return NULL;
}

void *wst_memrchr(const void *str, int c, size_t len)
{
    assert(len == 0 || str != NULL);

    if (len == 0)
        return NULL;

    const unsigned char *s = (const unsigned char *)str;

    size_t i = len;
    while (i--) {
        if (s[i] == (unsigned char)c)
            return (void *)(s + i);
    };

    return NULL;
}

char *wst_strchr(const char *str, int c)
{
    assert(str != NULL);

    unsigned char *p;
    for (p = (unsigned char *)str; *p != '\0'; p++) {
        if (*p == (unsigned char)c)
            return (char *)p;
    }

    if (c == 0)
        return (char *)p;

    return NULL;
}

char *wst_strrchr(const char *str, int c)
{
    return wst_memrchr(str, c, wst_strlen(str) + 1);
}

/*
 * First blind attempt on doing something strtok-like.
 * Quite far from posix strtok, so thrown away, has some design problems.
 * Main differences:
 * - delimiter is a single substring searched in input, instead of list of one char delims.
 * - does not skip multiple subsequent delimiters
 * - tries to restore char that was replaced with `\0`
 */
static char *my_strtok_custom(char *restrict str, const char *restrict delim)
{
    assert(delim != NULL);

    /** Position inside `str` that is computed for one strtok call ahead */
    static char *next_tok = NULL;
    /** Position of delimiter start inside `str` that was found during previous call */
    static char *last_delim = NULL;
    /** Orig first character of delimiter that was overwritten on previous call */
    static char delim_c = '\0';

    if (str != NULL) {
        next_tok = str;
        last_delim = NULL;
    }

    /* Restore first delim char that was replaced by '\0' at previous call. */
    if (last_delim != NULL) {
        *last_delim = delim_c;
        last_delim = NULL;
    }

    if (next_tok == NULL)
        return NULL;

    char *ret = next_tok;

    const char *delim_now = delim;
    char *start = next_tok;
    char *p = next_tok;

    while (1) {
        if (*p == '\0') {
            next_tok = NULL;
            break;
        }

        if (*delim_now == '\0') {
            next_tok = p;
            last_delim = start;
            delim_c = *start;
            *start = '\0';
            break;
        }

        if (*(p++) != *(delim_now++)) {
            delim_now = delim;
            start = p;
        }
    }

    return ret;
}

static bool is_delim(char c, const char *delim)
{
    for (const char *now = delim; *now != '\0'; now++) {
        if (c == *now)
            return true;
    }
    return false;
}

char *wst_strtok(char *restrict str, const char *restrict delim)
{
    assert(delim != NULL);

    /** Next position in string from where last call stopped */
    static char *next_tok = NULL;

    if (str != NULL)
        next_tok = str;

    if (next_tok == NULL)
        return NULL;

    /* Find next non-delimiter */
    char *token_start = next_tok;
    while (*token_start != '\0' && is_delim(*token_start, delim))
        token_start++;

    if (*token_start == '\0') {
        next_tok = NULL;
        return NULL;
    }

    /* Find next delimiter */
    char *p = token_start;
    while (*p != '\0' && !is_delim(*p, delim))
        p++;

    if (*p == '\0') {
        next_tok = NULL;
    } else {
        *p = '\0';
        next_tok = p + 1;
    }

    return token_start;
}
