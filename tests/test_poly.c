#include "toys/solve.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static int failures = 0;
static int counter = 0;

static void check_roots(const ToysPoly *poly, const ToysSolution *s, const ToysSolution *expected)
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

static void check_poly(ToysPoly *poly, const ToysSolution *expected)
{
    ToysSolution sol = toys_poly_solve(poly);
    check_roots(poly, &sol, expected);
}

static void test_poly(void)
{
    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { 6, -5, 1 } },
        &(ToysSolution){ .count = 2, .roots = { 2, 3 } } );
    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { -6, 5, -1 } },
        &(ToysSolution){ .count = 2, .roots = { 2, 3 } } );

    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { 1, -2, 1 } },
        &(ToysSolution){ .count = 1, .roots = { 1 } } );
    
    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { 1, 0, 1 } },
        &(ToysSolution){ .count = 0 } );
    
    check_poly(&(ToysPoly){ .degree = 1, .coeffs = { 2, -1 } },
        &(ToysSolution){ .count = 1, .roots = { 2 } } );

    check_poly(&(ToysPoly){ .degree = 0, .coeffs = { 5 } },
        &(ToysSolution){ .count = 0 } );

    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { 6, -5, 0 } },
        &(ToysSolution){ .count = 1, .roots = { 1.2 } } );

    check_poly(&(ToysPoly){ .degree = 2, .coeffs = { 0 } },
        &(ToysSolution){ .count = TOYS_SOLVE_INF } );

    check_poly(&(ToysPoly){ .degree = TOYS_POLY_MAX_DEGREE + 1, .coeffs = { 0 } },
        &(ToysSolution){ .count = TOYS_SOLVE_ERR } );
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
