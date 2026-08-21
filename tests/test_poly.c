#include "toys/solve.h"

#include <stdbool.h>
#include <stdio.h>

static int failures = 0;
static int counter = 0;

static void check_roots(const WstPoly *poly, const WstSolution *s, const WstSolution *expected)
{
    counter++;
    bool ok = s->count == expected->count;
    for (int i = 0; ok && i < s->count; i++)
        ok = iszero(s->roots[i] - expected->roots[i]);
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
    check_poly(&(WstPoly){ .degree = 2, .coeffs = { 6, -5, 1 } },
        &(WstSolution){ .count = 2, .roots = { 2, 3 } } );
    check_poly(&(WstPoly){ .degree = 2, .coeffs = { -6, 5, -1 } },
        &(WstSolution){ .count = 2, .roots = { 2, 3 } } );

    check_poly(&(WstPoly){ .degree = 2, .coeffs = { 1, -2, 1 } },
        &(WstSolution){ .count = 1, .roots = { 1 } } );
    
    check_poly(&(WstPoly){ .degree = 2, .coeffs = { 1, 0, 1 } },
        &(WstSolution){ .count = 0 } );
    
    check_poly(&(WstPoly){ .degree = 1, .coeffs = { 2, -1 } },
        &(WstSolution){ .count = 1, .roots = { 2 } } );

    check_poly(&(WstPoly){ .degree = 0, .coeffs = { 5 } },
        &(WstSolution){ .count = 0 } );

    check_poly(&(WstPoly){ .degree = 2, .coeffs = { 6, -5, 0 } },
        &(WstSolution){ .count = 1, .roots = { 1.2 } } );

    check_poly(&(WstPoly){ .degree = 2, .coeffs = { 0 } },
        &(WstSolution){ .count = WST_SOLVE_INF } );

    check_poly(&(WstPoly){ .degree = WST_SOLVE_MAX_DEGREE + 1, .coeffs = { 0 } },
        &(WstSolution){ .count = WST_SOLVE_ERR } );
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
