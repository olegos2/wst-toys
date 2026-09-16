#include "tests_common.h"

#include "toys/debug.h"
#include "toys/sort.h"

#include <string.h>

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
        int input[10];
        size_t count;
        int expected[10];
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
        int work[10] = { 0 };
        memcpy(work, cases[i].input, sizeof(work));
        wst_qsort(work, cases[i].count, sizeof(int), cmp_int);
        if (memcmp(work, cases[i].expected, sizeof(work)) != 0) {
            FAIL("qsort int test #%zu", i + 1);
            print_ints(work, cases[i].count, "Got     ");
            print_ints(cases[i].expected, cases[i].count, "Expected");
        }

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
        double input[6];
        size_t count;
        double expected[6];
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
        double work[6] = { 0 };
        memcpy(work, cases[i].input, sizeof(work));
        wst_qsort(work, cases[i].count, sizeof(double), cmp_double);
        bool ok = true;
        for (size_t j = 0; j < cases[i].count; j++) {
            if (!my_iszero(work[j] - cases[i].expected[j])) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            FAIL("qsort double test #%zu", i + 1);
            fprintf(stderr, "Got:     ");
            for (size_t j = 0; j < cases[i].count; j++)
                fprintf(stderr, " %lg", work[j]);
            fprintf(stderr, "\nExpected:");
            for (size_t j = 0; j < cases[i].count; j++)
                fprintf(stderr, " %lg", cases[i].expected[j]);
            fprintf(stderr, "\n");
        }
    }
}

static void test_sort_edge(void)
{
    /* count 0 / 1 and NULL must not crash */
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
    test_sort_edge();
    return tests_summary();
}
