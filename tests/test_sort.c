#include "tests_common.h"

#include "toys/debug.h"
#include "toys/sort.h"
#include "toys/string.h"

#include <string.h>

#define TEST_SORT_CAP 10

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static void print_ints(const int *arr, size_t n, const char *name)
{
    fprintf(stderr, "%s:", name);
    for (size_t i = 0; i < n; i++)
        fprintf(stderr, " %d", arr[i]);
    fprintf(stderr, "\n");
}

static void test_sort_int(void)
{
    static struct {
        int input[TEST_SORT_CAP];
        size_t count;
        int expected[TEST_SORT_CAP];
    } cases[] = {
        /* empty */
        {
            .input = { 0 },
            .count = 0,
            .expected = { 0 },
        },
        /* single */
        {
            .input = { 42 },
            .count = 1,
            .expected = { 42 },
        },
        /* already sorted */
        {
            .input = { 1, 2, 3, 4 },
            .count = 4,
            .expected = { 1, 2, 3, 4 }
        },
        /* reverse sorted */
        {
            .input = { 4, 3, 2, 1 },
            .count = 4,
            .expected = { 1, 2, 3, 4 },
        },
        /* duplicates */
        {
            .input = { 3, 1, 3, 2, 1 },
            .count = 5,
            .expected = { 1, 1, 2, 3, 3 },
        },
        /* negatives mixed */
        {
            .input = { 0, -5, 3, -2, 1 },
            .count = 5,
            .expected = { -5, -2, 0, 1, 3 },
        },
        /* all equal */
        {
            .input = { 7, 7, 7, 7 },
            .count = 4,
            .expected = { 7, 7, 7, 7 },
        },
        /* larger mixed */
        {
            .input = { 5, -1, 4, 0, 3, -3, 2, 1, -2, 4 },
            .count = 10,
            .expected = { -3, -2, -1, 0, 1, 2, 3, 4, 4, 5 },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        LOG_D("test int qsort #%zu", i + 1);

        int work[TEST_SORT_CAP] = { 0 };
        memcpy(work, cases[i].input, sizeof(work));
        wst_qsort(work, cases[i].count, sizeof(int), cmp_int);
        if (memcmp(work, cases[i].expected, sizeof(work)) != 0) {
            FAIL("qsort int test #%zu", i + 1);
            print_ints(work, cases[i].count, "Got     ");
            print_ints(cases[i].expected, cases[i].count, "Expected");
        }

        LOG_D("test int bsort #%zu", i + 1);

        memcpy(work, cases[i].input, sizeof(work));
        wst_bsort(work, cases[i].count, sizeof(int), cmp_int);
        if (memcmp(work, cases[i].expected, sizeof(work)) != 0) {
            FAIL("bsort int test #%zu", i + 1);
            print_ints(work, cases[i].count, "Got     ");
            print_ints(cases[i].expected, cases[i].count, "Expected");
        }
    }
}

static void test_sort_double(void)
{
    static struct {
        double input[TEST_SORT_CAP];
        size_t count;
        double expected[TEST_SORT_CAP];
    } cases[] = {
        /* reverse sorted, exercises non-int element size */
        {
            .input = { 3.5, 2.5, 1.5 },
            .count = 3,
            .expected = { 1.5, 2.5, 3.5 }
        },
        /* duplicates + negatives */
        {
            .input = { 0.0, -1.5, 2.0, -1.5, 1.0 },
            .count = 5,
            .expected = { -1.5, -1.5, 0.0, 1.0, 2.0 },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        LOG_D("test double qsort #%zu", i + 1);

        double work[TEST_SORT_CAP] = { 0 };
        memcpy(work, cases[i].input, sizeof(work));
        wst_qsort(work, cases[i].count, sizeof(double), cmp_double);

        for (size_t j = 0; j < cases[i].count; j++) {
            if (my_iszero(work[j] - cases[i].expected[j]))
                continue;
        
            FAIL("qsort double test #%zu", i + 1);
            fprintf(stderr, "Got:     ");
            for (size_t istr = 0; istr < cases[i].count; istr++)
                fprintf(stderr, " %lg", work[istr]);
            fprintf(stderr, "\nExpected:");
            for (size_t istr = 0; istr < cases[i].count; istr++)
                fprintf(stderr, " %lg", cases[i].expected[istr]);
            fprintf(stderr, "\n");
            break;
        }
    }
}

static int cmp_str(const void *a, const void *b)
{
    const char *x = *(char *const *)a;
    const char *y = *(char *const *)b;
    return wst_strcmp(x, y);
}

static void print_strs(const char *const *arr, size_t n, const char *name)
{
    fprintf(stderr, "%s:", name);
    for (size_t i = 0; i < n; i++)
        fprintf(stderr, " \"%s\"", arr[i]);
    fprintf(stderr, "\n");
}

static void test_sort_str(const char *const *input, size_t count,
                          const char *const *expected, WstSortFunc sort,
                          const char *name, size_t test_idx)
{
    LOG_D("test string %s #%zu", name, test_idx + 1);

    char *work[TEST_SORT_CAP] = { 0 };
    for (size_t istr = 0; istr < count; istr++)
        work[istr] = (char *)input[istr];

    sort(work, count, sizeof(char *), cmp_str);

    for (size_t istr = 0; istr < count; istr++) {

        if (wst_strcmp(work[istr], expected[istr]) == 0)
            continue;

        FAIL("string %s test #%zu", name, test_idx + 1);
        print_strs((const char *const *)work, count, "Got     ");
        print_strs(expected, count, "Expected");
        break;
    }
}

static void test_sort_strings(void)
{
    static struct {
        const char *input[TEST_SORT_CAP];
        size_t count;
        const char *expected[TEST_SORT_CAP];
    } cases[] = {
        /* basic */
        {
            .input = { "banana", "apple", "cherry" },
            .count = 3,
            .expected = { "apple", "banana", "cherry" },
        },
        /* prefixes and empty string come first */
        {
            .input = { "b", "ab", "", "abc", "a", "aa" },
            .count = 6,
            .expected = { "", "a", "aa", "ab", "abc", "b" },
        },
        /* uppercase before lowercase, duplicates kept */
        {
            .input = { "bob", "Alice", "alice", "Bob" },
            .count = 4,
            .expected = { "Alice", "Bob", "alice", "bob" },
        },
        /* reverse sorted */
        {
            .input = { "e", "d", "c", "b", "a" },
            .count = 5,
            .expected = { "a", "b", "c", "d", "e" },
        },
        /* duplicates */
        {
            .input = { "x", "a", "x", "m", "a" },
            .count = 5,
            .expected = { "a", "a", "m", "x", "x" },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        /* string sort via strcmp comparator */
        test_sort_str(cases[i].input, cases[i].count, cases[i].expected,
                  wst_qsort, "qsort", i);
        test_sort_str(cases[i].input, cases[i].count, cases[i].expected,
                  wst_bsort, "bsort", i);

        /* specialized bucket sort for strings */
        {
            LOG_D("test string bucket #%zu", i + 1);

            const char *work[TEST_SORT_CAP] = { 0 };
            for (size_t j = 0; j < cases[i].count; j++)
                work[j] = cases[i].input[j];

            wst_str_sort(work, cases[i].count);

            for (size_t j = 0; j < cases[i].count; j++) {
                if (wst_strcmp(work[j], cases[i].expected[j]) == 0)
                    continue;

                FAIL("string str_sort test #%zu", i + 1);
                print_strs((const char *const *)work, cases[i].count, "Got     ");
                print_strs(cases[i].expected, cases[i].count, "Expected");
                break;
            }
        }
    }

    /* count < 2 must not crash. */
    wst_str_sort(NULL, 0);
    const char *one[] = { "solo" };
    wst_str_sort(one, 1);
    CHECK(wst_strcmp(one[0], "solo") == 0, "single string unchanged");
}

static void test_sort_edge(void)
{
    /* count < 2 and NULL must not crash */
    wst_qsort(NULL, 0, sizeof(int), cmp_int);
    int one = 5;
    wst_qsort(&one, 1, sizeof(int), cmp_int);
    CHECK(one == 5, "single element unchanged");
}

int main(void)
{
    wst_log_set_max_prio(WST_LOG_VERBOSE);
    test_sort_int();
    test_sort_double();
    test_sort_strings();
    test_sort_edge();
    return tests_summary();
}
