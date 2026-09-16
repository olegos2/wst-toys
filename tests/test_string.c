#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for memrchr */
#endif

#include "tests_common.h"

#include "toys/string.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STR_RET(ret_type, ret_fmt, func, str, ...) { \
    ret_type wst_ret = wst_ ## func(str, ##__VA_ARGS__); \
    ret_type ret = func(str, ##__VA_ARGS__); \
    CHECK(wst_ret == ret, #func "(%s, ...): Got " ret_fmt ", expected " ret_fmt, str, wst_ret, ret); \
}

#define STRINGS_SET_0 "Hello, world!"
#define STRINGS_SET_0_SUB "Hello"

void test_strlen(void)
{
    TEST_STR_RET(size_t, "%zu", strlen, STRINGS_SET_0);
    TEST_STR_RET(size_t, "%zu", strnlen,
                 STRINGS_SET_0, sizeof(STRINGS_SET_0_SUB) - 1);
}

void test_strcpy(void)
{
    static const struct {
        const char *src;
        const char *result;
    } cases[] = {
        {
            .src = STRINGS_SET_0,
            .result = STRINGS_SET_0,
        }
    };
    
    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        char ret[strlen(cases[i].result) + 1];
        wst_strcpy(ret, cases[i].src);

        if (strcmp(cases[i].result, ret) != 0)
            FAIL("#%zu: strcpy(..., %s) -> %s, expected %s",
                 i, cases[i].src, ret, cases[i].result);
    }
}

void test_strncpy(void)
{
    static const struct {
        const char *src;
        size_t nbytes;
        const char *result;
    } cases[] = {
        {
            .src = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0),
            .result = STRINGS_SET_0,
        },
        {
            .src = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0_SUB),
            .result = STRINGS_SET_0_SUB,
        }
    };
    
    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        char ret[strlen(cases[i].result) + 1];
        wst_strncpy(ret, cases[i].src, cases[i].nbytes);

        if (strcmp(cases[i].result, ret) != 0)
            FAIL("#%zu: strcpy(..., %s, %zu) -> %s, expected %s",
                 i, cases[i].src, cases[i].nbytes, ret, cases[i].result);
    }
}

void test_strdup(void)
{
    static const struct {
        const char *src;
        const char *result;
    } cases[] = {
        {
            .src = STRINGS_SET_0,
            .result = STRINGS_SET_0,
        }
    };
    
    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        const char *ret = wst_strdup(cases[i].src);
        if (!ret) {
            FAIL("#%zu: strdup(%s) -> NULL, expected %s", i, cases[i].src, cases[i].result);
        } else {
            if (strcmp(cases[i].result, ret) != 0)
                FAIL("#%zu: strdup(%s) -> %s, expected %s", i, cases[i].src, ret, cases[i].result);
            free((void *)ret);
        }
    }
}

void test_strndup(void)
{
    static const struct {
        const char *src;
        size_t nbytes;
        const char *result;
    } cases[] = {
        {
            .src = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0) - 1,
            .result = STRINGS_SET_0,
        },
        {
            .src = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0_SUB) - 1,
            .result = STRINGS_SET_0_SUB,
        }
    };
    
    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        const char *ret = wst_strndup(cases[i].src, cases[i].nbytes);
        if (!ret) {
            FAIL("#%zu: strndup(%s, %zu) -> NULL, expected %s",
                 i, cases[i].src, cases[i].nbytes, cases[i].result);
        } else {
            if (strcmp(cases[i].result, ret) != 0)
                FAIL("#%zu: strndup(%s, %zu): %s, expected %s",
                     i, cases[i].src, cases[i].nbytes, ret, cases[i].result);
            free((void *)ret);
        }
    }
}

static int sign(int x)
{
    if (x == 0) return 0;
    if (x < 0) return -1;
    return 1;
}

void test_strcmp(void)
{
    static const struct {
        const char *str1;
        const char *str2;
    } cases[] = {
        {
            .str1 = STRINGS_SET_0,
            .str2 = STRINGS_SET_0,
        },
        {
            .str1 = STRINGS_SET_0_SUB,
            .str2 = STRINGS_SET_0,
        },
        {
            .str1 = STRINGS_SET_0,
            .str2 = STRINGS_SET_0_SUB,
        },
        {
            .str1 = "Aba Caba",
            .str2 = "aba caba",
        },
        {
            .str1 = "caba faba",
            .str2 = "aba caba",
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        int wst_ret = sign(wst_strcmp(cases[i].str1, cases[i].str2));
        int ret = sign(strcmp(cases[i].str1, cases[i].str2));
        CHECK(wst_ret == ret, "#%zu: strcmp(%s, %s): Got %d, expected %d",
              i, cases[i].str1, cases[i].str2, wst_ret, ret);
    }
}  

void test_strncmp(void)
{
    static const struct {
        const char *str1;
        const char *str2;
        size_t nbytes;
    } cases[] = {
        {
            .str1 = STRINGS_SET_0,
            .str2 = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0) + 100,
        },
        {
            .str1 = STRINGS_SET_0_SUB,
            .str2 = STRINGS_SET_0,
            .nbytes = sizeof(STRINGS_SET_0_SUB),
        },
        {
            .str1 = STRINGS_SET_0,
            .str2 = STRINGS_SET_0_SUB,
            .nbytes = sizeof(STRINGS_SET_0),
        },
        {
            .str1 = "Aba Caba",
            .str2 = "aba caba",
            .nbytes = 0x100,
        },
        {
            .str1 = "caba faba",
            .str2 = "aba caba",
            .nbytes = 0x100,
        },
        {
            .str1 = "aba caba",
            .str2 = "aba ainfaoif",
            .nbytes = 0x100,
        },
        {
            .str1 = "aba caba",
            .str2 = "aba ainfaoif",
            .nbytes = 4,
        }
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        int wst_ret = sign(wst_strncmp(cases[i].str1, cases[i].str2, cases[i].nbytes));
        int ret = sign(strncmp(cases[i].str1, cases[i].str2, cases[i].nbytes));
        CHECK(wst_ret == ret, "#%zu: strncmp(%s, %s, %zu): Got %d, expected %d",
              i, cases[i].str1, cases[i].str2, cases[i].nbytes, wst_ret, ret);
    }

}

void test_strcat(void)
{
    static const struct {
        const char *dest;
        const char *src;
        const char *result;
    } cases[] = {
        {
            .dest = "Hello, ",
            .src = "world!",
            .result = "Hello, world!",
        },
        {
            .dest = "",
            .src = "abc",
            .result = "abc",
        },
        {
            .dest = "abc",
            .src = "",
            .result = "abc",
        },
        {
            .dest = "",
            .src = "",
            .result = "",
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        char ret[strlen(cases[i].dest) + strlen(cases[i].src) + 1];
        strcpy(ret, cases[i].dest);
        wst_strcat(ret, cases[i].src);

        if (strcmp(cases[i].result, ret) != 0)
            FAIL("#%zu: strcat(%s, %s) -> %s, expected %s",
                 i, cases[i].dest, cases[i].src, ret, cases[i].result);
    }
}

void test_strchr(void)
{
    static const struct {
        const char *str;
        int c;
    } cases[] = {
        { .str = STRINGS_SET_0, .c = 'H' },
        { .str = STRINGS_SET_0, .c = 'o' },
        { .str = STRINGS_SET_0, .c = '!' },
        { .str = STRINGS_SET_0, .c = '\0' },
        { .str = STRINGS_SET_0, .c = 'z' },
        { .str = "", .c = '\0' },
        { .str = "", .c = 'a' },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        char *wst_ret = wst_strchr(cases[i].str, cases[i].c);
        const char *ret = strchr(cases[i].str, cases[i].c);
        CHECK(wst_ret == ret, "#%zu: strchr(%s, %d): Got %p, expected %p",
              i, cases[i].str, cases[i].c, (void *)wst_ret, (void *)ret);

        char *wst_rret = wst_strrchr(cases[i].str, cases[i].c);
        const char *rret = strrchr(cases[i].str, cases[i].c);
        CHECK(wst_rret == rret, "#%zu: strrchr(%s, %d): Got %p, expected %p",
              i, cases[i].str, cases[i].c, (void *)wst_rret, (void *)rret);
    }
}

void test_memchr(void)
{
    static const struct {
        const char *str;
        int c;
        size_t nbytes;
    } cases[] = {
        { .str = STRINGS_SET_0, .c = 'H', .nbytes = sizeof(STRINGS_SET_0) },
        { .str = STRINGS_SET_0, .c = 'o', .nbytes = sizeof(STRINGS_SET_0) },
        { .str = STRINGS_SET_0, .c = '\0', .nbytes = sizeof(STRINGS_SET_0) },
        { .str = STRINGS_SET_0, .c = 'z', .nbytes = sizeof(STRINGS_SET_0) },
        /* bounded search must not see past nbytes */
        { .str = STRINGS_SET_0, .c = 'w', .nbytes = 5 },
        { .str = STRINGS_SET_0, .c = 'H', .nbytes = 0 },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        void *wst_ret = wst_memchr(cases[i].str, cases[i].c, cases[i].nbytes);
        const void *ret = memchr(cases[i].str, cases[i].c, cases[i].nbytes);
        CHECK(wst_ret == ret, "#%zu: memchr(%s, %d, %zu): Got %p, expected %p",
              i, cases[i].str, cases[i].c, cases[i].nbytes, wst_ret, ret);

        void *wst_rret = wst_memrchr(cases[i].str, cases[i].c, cases[i].nbytes);
        void *rret = memrchr(cases[i].str, cases[i].c, cases[i].nbytes);
        CHECK(wst_rret == rret, "#%zu: memrchr(%s, %d, %zu): Got %p, expected %p",
              i, cases[i].str, cases[i].c, cases[i].nbytes, wst_rret, rret);
    }
}

void test_strtok(void)
{
    static const struct {
        const char *input;
        const char *delim;
        const char *expected[4];
        size_t exp_count;
    } cases[] = {
        {
            .input = "a,b,c",
            .delim = ",",
            .expected = { "a", "b", "c" },
            .exp_count = 3,
        },
        {
            /* leading/trailing/repeated delims are skipped */
            .input = "  hello   world  ",
            .delim = " ",
            .expected = { "hello", "world" },
            .exp_count = 2,
        },
        {
            .input = "abc",
            .delim = ",",
            .expected = { "abc" },
            .exp_count = 1,
        },
        {
            .input = "",
            .delim = ",",
            .expected = { NULL },
            .exp_count = 0,
        },
        {
            /* string of only delimiters yields no tokens */
            .input = ",,,",
            .delim = ",",
            .expected = { NULL },
            .exp_count = 0,
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        char buf[64];
        strcpy(buf, cases[i].input);

        size_t count = 0;
        bool ok = true;
        char *tok = wst_strtok(buf, cases[i].delim);
        for (; tok != NULL; tok = wst_strtok(NULL, cases[i].delim)) {
            if (count >= cases[i].exp_count || strcmp(tok, cases[i].expected[count]) != 0) {
                ok = false;
                break;
            }
            count++;
        }
        if (!ok || count != cases[i].exp_count) {
            FAIL("#%zu: strtok(%s, %s) -> %zu tokens, expected %zu",
                 i, cases[i].input, cases[i].delim, count, cases[i].exp_count);
        }
    }
}

int main(void)
{
    test_strlen();
    test_strcpy();
    test_strdup();
    test_strndup();
    test_strcmp();
    test_strncmp();
    test_strcat();
    test_strchr();
    test_memchr();
    test_strtok();

    return tests_summary();
}
