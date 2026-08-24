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

    fprintf(stderr, "\nPolynomial degree %d failed (test %d):\n", poly->degree, counter);
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

static void test_poly(void)
{
    static WstPoly test_inputs[] = {
        { .degree = 2, .coeffs = { 6, -5, 1 } },
        { .degree = 2, .coeffs = { -6, 5, -1 } },
        { .degree = 2, .coeffs = { 1, -2, 1 } },
        { .degree = 2, .coeffs = { 1, 0, 1 } },
        { .degree = 1, .coeffs = { 2, -1 } },
        { .degree = 0, .coeffs = { 5 } },
        { .degree = 2, .coeffs = { 6, -5, 0 } },
        { .degree = 2, .coeffs = { 0 } },
        { .degree = WST_SOLVE_MAX_DEGREE + 1, .coeffs = { 0 } },
    };
    static const WstSolution test_outputs[] = {
        { .count = 2, .roots = { 2, 3 } },
        { .count = 2, .roots = { 2, 3 } },
        { .count = 1, .roots = { 1 } },
        { .count = 0 },
        { .count = 1, .roots = { 2 } },
        { .count = 0 },
        { .count = 1, .roots = { 1.2 } },
        { .count = WST_SOLVE_INF },
        { .count = WST_SOLVE_ERR },
    };

    assert(ARR_LEN(test_inputs) == ARR_LEN(test_outputs));

    for (size_t i = 0; i < ARR_LEN(test_inputs); i++) {
        check_poly(&test_inputs[i], &test_outputs[i]);
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
