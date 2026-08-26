#include "toys/solve.h"
#include "toys/math.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

static int failures = 0;

static void check_roots(const WstPoly *poly, const WstSolution *s, const WstSolution *expected)
{
    static int counter = 0;
    counter++;
    bool ok = s->count == expected->count;
    for (int i = 0; i < s->count; i++) {
        if (!ok) break;
        ok = my_iszero(s->roots[i] - expected->roots[i]);
    }
    if (ok) return;

    fprintf(stderr, "\nFAIL polynomial degree %d (test %d):\n", poly->degree, counter);
    for (int i = 0; i < poly->degree; i++)
        fprintf(stderr, "[%d] = %lg,\n", i, poly->coeffs[i]);
    fprintf(stderr, "Got sol %d roots:\n", s->count);
    for (int i = 0; i < s->count; i++)
        fprintf(stderr, "[%d] = %lg,\n", i, s->roots[i]);
    fprintf(stderr, "Expected sol %d roots:\n", expected->count);
    for (int i = 0; i < expected->count; i++)
        fprintf(stderr, "[%d] = %lg,\n", i, expected->roots[i]);

    failures++;
}

static void check_poly(WstPoly *poly, const WstSolution *expected)
{
    WstSolution sol = wst_solve_poly(poly);
    check_roots(poly, &sol, expected);
}

typedef struct {
    WstPoly poly;
    WstSolution sol;
} TestCase;

static void test_poly(void)
{
    static TestCase cases[] = {
        {
            .poly = { .degree = 2, .coeffs = { 6, -5, 1 } },
            .sol = { .count = 2, .roots = { 2, 3 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { -6, 5, -1 } },
            .sol = { .count = 2, .roots = { 2, 3 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 1, -2, 1 } },
            .sol = { .count = 1, .roots = { 1 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 1, 0, 1 } },
            .sol = { .count = 0 },
        },
        {
            .poly = { .degree = 1, .coeffs = { 2, -1 } },
            .sol = { .count = 1, .roots = { 2 } },
        },
        {
            .poly = { .degree = 0, .coeffs = { 5 } },
            .sol = { .count = 0 },
        },
        {
            .poly = { .degree = 2, .coeffs = { 6, -5, 0 } },
            .sol = { .count = 1, .roots = { 1.2 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 0 } },
            .sol = { .count = WST_SOLVE_INF },
        },
        {
            .poly = { .degree = WST_SOLVE_MAX_DEGREE + 1, .coeffs = { 0 } },
            .sol = { .count = WST_SOLVE_ERR },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        check_poly(&cases[i].poly, &cases[i].sol);
    }
}

int main(void)
{
    test_poly();
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
