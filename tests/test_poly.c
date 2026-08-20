#include "toys/solve.h"

#include <math.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

#define CHECK_DBL(a, b, msg) \
    do { \
        double __a = (a); \
        double __b = (b); \
        if (!(fabs(__a - __b) <= 1e-6)) { \
            fprintf(stderr, "FAIL %s: got %g want %g (%s:%d)\n", \
                    msg, __a, __b, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

static void test_quad(void)
{
    double x1, x2;

    CHECK(toys_quad_solve(1, -5, 6, &x1, &x2) == 2, "quad two roots count");
    CHECK_DBL(x1, 2.0, "quad x1");
    CHECK_DBL(x2, 3.0, "quad x2");

    CHECK(toys_quad_solve(1, -2, 1, &x1, &x2) == 1, "quad double count");
    CHECK_DBL(x1, 1.0, "quad double x1");

    CHECK(toys_quad_solve(1, 0, 1, &x1, &x2) == 0, "quad no roots");

    CHECK(toys_quad_solve(0, 2, -4, &x1, &x2) == 1, "linear count");
    CHECK_DBL(x1, 2.0, "linear x1");

    CHECK(toys_quad_solve(0, 0, 0, &x1, &x2) == TOYS_SOLVE_INF, "quad inf any");
    CHECK(toys_quad_solve(0, 0, 5, &x1, &x2) == 0, "quad inf none");
}

static void check_roots(int n, const double *roots, const double *expected, int count)
{
    char msg[64];
    CHECK(n == count, "poly root count");
    for (int i = 0; i < n && i < count; i++) {
        snprintf(msg, sizeof(msg), "poly root[%d]", i);
        CHECK_DBL(roots[i], expected[i], msg);
    }
}

static void test_poly(void)
{
    double roots[TOYS_POLY_MAX_DEGREE + 1];
    int count;

    /* x^2 - 5x + 6 = 0 */
    static const double quad_expected[] = { 2, 3 };
    ToysPoly quad = { .degree = 2, .coeffs = { 6, -5, 1 } };
    count = toys_poly_solve(&quad, roots, TOYS_POLY_MAX_DEGREE + 1);
    check_roots(count, roots, quad_expected, 2);

    /* -x^2 + 5x - 6 = 0, negative leading coefficient (x1 <= x2) */
    ToysPoly neg = { .degree = 2, .coeffs = { -6, 5, -1 } };
    count = toys_poly_solve(&neg, roots, TOYS_POLY_MAX_DEGREE + 1);
    check_roots(count, roots, quad_expected, 2);

    /* x^2 - 2x + 1 = 0, double root */
    static const double doubled_expected[] = { 1 };
    ToysPoly doubled = { .degree = 2, .coeffs = { 1, -2, 1 } };
    count = toys_poly_solve(&doubled, roots, TOYS_POLY_MAX_DEGREE + 1);
    check_roots(count, roots, doubled_expected, 1);

    /* x^2 + 1 = 0, no real roots */
    ToysPoly none = { .degree = 2, .coeffs = { 1, 0, 1 } };
    count = toys_poly_solve(&none, roots, TOYS_POLY_MAX_DEGREE + 1);
    CHECK(count == 0, "poly no roots");

    /* 2 - x = 0 */
    static const double linear_expected[] = { 2 };
    ToysPoly linear = { .degree = 1, .coeffs = { 2, -1 } };
    count = toys_poly_solve(&linear, roots, TOYS_POLY_MAX_DEGREE + 1);
    check_roots(count, roots, linear_expected, 1);

    /* 5 = 0 has no roots */
    ToysPoly constant = { .degree = 0, .coeffs = { 5 } };
    count = toys_poly_solve(&constant, roots, TOYS_POLY_MAX_DEGREE + 1);
    CHECK(count == 0, "poly constant none");

    /* trailing zeros are trimmed by the solver */
    static const double trimmed_expected[] = { 1.2 };
    ToysPoly trimmed = { .degree = 2, .coeffs = { 6, -5, 0 } };
    count = toys_poly_solve(&trimmed, roots, TOYS_POLY_MAX_DEGREE + 1);
    check_roots(count, roots, trimmed_expected, 1);

    /* all-zero: every x is a solution */
    ToysPoly allzero = { .degree = 2, .coeffs = { 0 } };
    CHECK(toys_poly_solve(&allzero, roots, TOYS_POLY_MAX_DEGREE + 1) ==
          TOYS_SOLVE_INF, "poly all-zero inf");

    /* degree above the limit is an error */
    ToysPoly too_big = { .degree = TOYS_POLY_MAX_DEGREE + 1, .coeffs = { 0 } };
    CHECK(toys_poly_solve(&too_big, roots, TOYS_POLY_MAX_DEGREE + 1) ==
          TOYS_SOLVE_ERR, "poly degree too big");
}

int main(void)
{
    test_quad();
    test_poly();
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}