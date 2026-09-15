#include "tests_common.h"

#include "toys/string.h"

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

}

void test_strchr(void)
{

}

void test_strtok(void)
{

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
    test_strtok();

    return tests_summary();
}
