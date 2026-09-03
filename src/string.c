#include "toys/string.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* TODO: Optimize some mem ops to use wider types */

size_t my_strlen(const char *str)
{
    assert(str != NULL);

    size_t i = 0;
    while (str[i] != '\0') i++;

    return i;
}

size_t my_strnlen(const char *str, size_t max_len)
{
    assert(str != NULL);

    size_t i = 0;
    while (str[i] != '\0' && i < max_len) i++;

    return i;
}

const char *my_strnul(const char *str)
{
    assert(str != NULL);

    const char *cur = str;
    while (*cur != '\0') cur++;

    return cur;
}

char *my_stpcpy(char *restrict dest, const char *restrict src)
{
    assert(dest != NULL);
    assert(src != NULL);

    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';

    return dest + i;
}

char *my_stpncpy(char *restrict dest, const char *restrict src, size_t dsize)
{
    assert(dest != NULL);
    assert(src != NULL);

    size_t i;
    for (i = 0; src[i] != '\0' && i < dsize; i++)
        dest[i] = src[i];
    dest[i] = '\0';

    return dest + i;
}

char *my_strcpy(char *restrict dest, const char *restrict src)
{
    my_stpcpy(dest, src);

    return dest;
}

char *my_strncpy(char *restrict dest, const char *restrict src, size_t dsize)
{
    my_stpncpy(dest, src, dsize);

    return dest;
}

char *my_strdup(const char *str)
{
    assert(str != NULL);

    char *dest = calloc(my_strlen(str) + 1, sizeof(char));
    
    return my_strcpy(dest, str);
}

int my_memcmp(const void *str1, const void *str2, size_t len)
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

void *my_memcpy(void *restrict dest, const void *restrict src, size_t len)
{
    assert(dest != NULL);
    assert(src != NULL);

    for (size_t i = 0; i < len; i++) {
        ((unsigned char *)dest)[i] = ((unsigned char *)src)[i];
    }

    return dest;
}

int my_strcmp(const char *str1, const char *str2)
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

char *my_strcat(char *restrict dest, const char *restrict src)
{
    my_strcpy((char *)my_strnul(dest), src);

    return dest;
}

void *my_memchr(const void *str, int c, size_t len)
{
    assert(len == 0 || str != NULL);

    for (size_t i = 0; i < len; i++) {
        if (((unsigned char *)str)[i] == (unsigned char)c)
            return (void *)(str + i);
    }

    return NULL;
}

void *my_memrchr(const void *str, int c, size_t len)
{
    assert(len == 0 || str != NULL);

    for (size_t i = len - 1; i >= 0; i--) {
        if (((unsigned char *)str)[i] == (unsigned char)c)
            return (void *)(str + i);
    }

    return NULL;
}

char *my_strchr(const char *str, int c)
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

/*
 * First blind attempt on doing something strtok-like.
 * Quite far from posix strtok, so thrown away, has some design problems.
 * Main differences:
 * - delimiter is a single substring searched in input, instead of list of one char delims.
 * - does not skip multiple subsequent delimiters
 * - tries to restore char that was replaced with `\0`
 */
char *my_strtok_custom(char *restrict str, const char *restrict delim)
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

char *my_strtok(char *restrict str, const char *restrict delim)
{
    assert(delim != NULL);

    /** Position inside `str` that is computed for one strtok call ahead */
    static char *next_tok = NULL;

    if (str != NULL)
        next_tok = str;

    if (next_tok == NULL)
        return NULL;

    char *ret = next_tok;
    char *p;

    for (p = next_tok; *p != '\0'; p++) {
        if (is_delim(*p, delim)) {
            *p = '\0';
        }
    }

    for (char *p = next_tok; ; p++) {
        if (*p == '\0') {
            next_tok = NULL;
            break;
        }

        if (!is_delim(*p, delim)) {
            next_tok = p;
            break;
        }
    }

    return ret;
}
