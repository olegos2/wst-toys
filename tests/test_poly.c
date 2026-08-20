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

    static const double cubic[] = { -6, 11, -6, 1 };
    static const double cubic_expected[] = { 1, 2, 3 };
    check_roots(toys_poly_solve(3, cubic, roots, TOYS_POLY_MAX_DEGREE + 1),
                roots, cubic_expected, 3);

    static const double cube1[] = { -1, 0, 0, 1 };
    static const double cube1_expected[] = { 1 };
    check_roots(toys_poly_solve(3, cube1, roots, TOYS_POLY_MAX_DEGREE + 1),
                roots, cube1_expected, 1);

    static const double quartic[] = { 4, 0, -5, 0, 1 };
    static const double quartic_expected[] = { -2, -1, 1, 2 };
    check_roots(toys_poly_solve(4, quartic, roots, TOYS_POLY_MAX_DEGREE + 1),
                roots, quartic_expected, 4);

    static const double pow4[] = { 1, -4, 6, -4, 1 };
    static const double pow4_expected[] = { 1 };
    check_roots(toys_poly_solve(4, pow4, roots, TOYS_POLY_MAX_DEGREE + 1),
                roots, pow4_expected, 1);

    static const double pow6[] = { 1, -6, 15, -20, 15, -6, 1 };
    static const double pow6_expected[] = { 1 };
    check_roots(toys_poly_solve(6, pow6, roots, TOYS_POLY_MAX_DEGREE + 1),
                roots, pow6_expected, 1);

    static const double allzero[] = { 0, 0, 0, 0, 0, 0 };
    CHECK(toys_poly_solve(5, allzero, roots, TOYS_POLY_MAX_DEGREE + 1) ==
          TOYS_SOLVE_INF, "poly all-zero inf");
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
