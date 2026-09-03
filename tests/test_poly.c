#include "tests_common.h"

#include "toys/poly.h"
#include "toys/math.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static void test_solve(void)
{
    struct {
        WstPoly poly;
        WstSolution sol;
    } cases[] = {
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
            .poly = { .degree = 3, .coeffs = { 1, 2, 3, 4 } },
            .sol = { .count = WST_SOLVE_ERR },
        },
        {
            .poly = { .degree = 0, .coeffs = { 0 } },
            .sol = { .count = WST_SOLVE_INF },
        },
        {
            .poly = { .degree = 1, .coeffs = { 1, 1 } },
            .sol = { .count = 1, .roots = { -1 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { -2, 0, 1 } },
            .sol = { .count = 2, .roots = { -sqrt(2), sqrt(2) } },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        WstSolution sol = wst_poly_solve(&cases[i].poly);
        if (!cmp_solution(&sol, &cases[i].sol)) {
            FAIL("solve test #%zu", i + 1);
            print_poly(&cases[i].poly, "Polynomial");
            print_sol(&sol, "Got");
            print_sol(&cases[i].sol, "Expected");
        }
    }
}

static void test_trim(void)
{
    struct {
        WstPoly poly, expected;
    } cases[] = {
        {
            .poly = { .degree = 3, .coeffs = { 1, 2 } },
            .expected = { .degree = 1, .coeffs = { 1, 2 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 1, 2, 3 } },
            .expected = { .degree = 2, .coeffs = { 1, 2, 3 } },
        },
        {
            .poly = { .degree = 0, .coeffs = { 5 } },
            .expected = { .degree = 0, .coeffs = { 5 } },
        },
        {
            .poly = { .degree = 4, .coeffs = { 0, 0, 0, 0, 0 } },
            .expected = { .degree = 0, .coeffs = { 0 } },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        wst_poly_trim(&cases[i].poly);
        if (!wst_poly_cmp(&cases[i].poly, &cases[i].expected)) {
            FAIL("trim test #%zu", i + 1);
            print_poly(&cases[i].poly, "Got");
            print_poly(&cases[i].expected, "Expected");
        }
    }
}

static void test_scale(void)
{
    struct {
        WstPoly poly;
        double scale;
        WstPoly expected;
    } cases[] = {
        {
            .poly = { .degree = 2, .coeffs = { 1, 2, 3 } },
            .scale = 2.0,
            .expected = { .degree = 2, .coeffs = { 2, 4, 6 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 1, 2, 3 } },
            .scale = 0.0,
            .expected = { .degree = 0, .coeffs = { 0 } },
        },
        {
            .poly = { .degree = 2, .coeffs = { 2, 4, 6 } },
            .scale = -0.5,
            .expected = { .degree = 2, .coeffs = { -1, -2, -3 } },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        wst_poly_scale(&cases[i].poly, cases[i].scale);
        if (!wst_poly_cmp(&cases[i].poly, &cases[i].expected)) {
            FAIL("cmp test #%zu", i + 1);
            print_poly(&cases[i].poly, "Got");
            print_poly(&cases[i].expected, "Expected");
        }
    }
}

/* TODO: refactor the rest of tests. */

static void test_add(void)
{
    WstPoly poly;

    poly = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_add(&poly, &(WstPoly){ .degree = 2, .coeffs = { 4, 5, 6 } });
    CHECK(wst_poly_cmp(&poly, &(WstPoly){ .degree = 2, .coeffs = { 5, 7, 9 } }), "add same degree");

    poly = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    wst_poly_add(&poly, &(WstPoly){ .degree = 3, .coeffs = { 0, 0, 0, 1 } });
    CHECK(wst_poly_cmp(&poly, &(WstPoly){ .degree = 3, .coeffs = { 1, 2, 0, 1 } }), "add increasing degree");

    poly = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_add(&poly, &(WstPoly){ .degree = 2, .coeffs = { -1, -2, -3 } });
    CHECK(poly.degree == 0 && my_iszero(poly.coeffs[0]), "add cancels to zero");
}

static void test_sub(void)
{
    WstPoly a;

    a = (WstPoly){ .degree = 2, .coeffs = { 5, 7, 9 } };
    wst_poly_sub(&a, &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } });
    CHECK(wst_poly_cmp(&a, &(WstPoly){ .degree = 2, .coeffs = { 4, 5, 6 } }), "sub same degree");

    a = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    wst_poly_sub(&a, &(WstPoly){ .degree = 1, .coeffs = { 1, 2 } });
    CHECK(a.degree == 0 && my_iszero(a.coeffs[0]), "sub self to zero");
}

static void test_mul(void)
{
    WstPoly a;

    /* (1 + x) * (1 + x) = 1 + 2x + x^2 */
    a = (WstPoly){ .degree = 1, .coeffs = { 1, 1 } };
    CHECK(wst_poly_mul(&a, &(WstPoly){ .degree = 1, .coeffs = { 1, 1 } }), "mul succeeds");
    CHECK(wst_poly_cmp(&a, &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 1 } }), "mul (1+x)^2");

    /* 3 * x = 3x */
    a = (WstPoly){ .degree = 0, .coeffs = { 3 } };
    CHECK(wst_poly_mul(&a, &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } }), "mul const*x");
    CHECK(wst_poly_cmp(&a, &(WstPoly){ .degree = 1, .coeffs = { 0, 3 } }), "mul const*x result");

    /* mul overflow */
    // a = (WstPoly){ .degree = 5, .coeffs = { 1, 1, 1, 1, 1, 1 } };
    // CHECK(!wst_poly_mul(&a, &(WstPoly){ .degree = 5, .coeffs = { 1, 1, 1, 1, 1, 1 } }), "mul overflow");
}

static void test_cmp(void)
{
    WstPoly a = { .degree = 2, .coeffs = { 1, 2, 3 } };
    WstPoly b = { .degree = 2, .coeffs = { 1, 2, 3 } };
    CHECK(wst_poly_cmp(&a, &b), "cmp equal");

    b.coeffs[0] = 99;
    CHECK(!wst_poly_cmp(&a, &b), "cmp different coeff");

    a = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    b = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    CHECK(!wst_poly_cmp(&a, &b), "cmp different degree");
}

static void test_deriv(void)
{
    WstPoly p, d;

    /* deriv of x^2 + 2x + 1 => 2x + 2 */
    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 1 } };
    wst_poly_deriv(&p, &d);
    CHECK(wst_poly_cmp(&d, &(WstPoly){ .degree = 1, .coeffs = { 2, 2 } }), "deriv quadratic");

    /* deriv of constant => 0 */
    p = (WstPoly){ .degree = 0, .coeffs = { 5 } };
    wst_poly_deriv(&p, &d);
    CHECK(d.degree == 0 && my_iszero(d.coeffs[0]), "deriv constant");

    /* deriv of 3x^3 + x => 9x^2 + 1 */
    p = (WstPoly){ .degree = 3, .coeffs = { 0, 1, 0, 3 } };
    wst_poly_deriv(&p, &d);
    CHECK(wst_poly_cmp(&d, &(WstPoly){ .degree = 2, .coeffs = { 1, 0, 9 } }), "deriv cubic");
}

static void test_integ(void)
{
    WstPoly p, out;

    /* integ of 2x + 1 => x^2 + x + C (C=0) */
    p = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    CHECK(wst_poly_integ(&p, &out), "integ succeeds");
    CHECK(wst_poly_cmp(&out, &(WstPoly){ .degree = 2, .coeffs = { 0, 1, 1 } }), "integ linear");

    /* integ of constant 5 => 5x */
    p = (WstPoly){ .degree = 0, .coeffs = { 5 } };
    CHECK(wst_poly_integ(&p, &out), "integ constant");
    CHECK(wst_poly_cmp(&out, &(WstPoly){ .degree = 1, .coeffs = { 0, 5 } }), "integ constant result");

    /* integ overflow: degree MAX-1 => degree MAX succeeds, degree MAX => fail */
    p = (WstPoly){ .degree = WST_POLY_MAX_DEGREE - 1, .coeffs = { 1 } };
    CHECK(wst_poly_integ(&p, &out), "integ at max-1");
    CHECK(out.degree == WST_POLY_MAX_DEGREE, "integ degree at max");

    p = (WstPoly){ .degree = WST_POLY_MAX_DEGREE, .coeffs = { 1 } };
    CHECK(!wst_poly_integ(&p, &out), "integ overflows");
}

static void test_eval(void)
{
    WstPoly p;

    /* P(x) = 1 + 2x + 3x^2, P(0) = 1 */
    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    CHECK(my_iszero(wst_poly_eval(&p, 0.0) - 1.0), "eval at 0");

    /* P(1) = 1 + 2 + 3 = 6 */
    CHECK(my_iszero(wst_poly_eval(&p, 1.0) - 6.0), "eval at 1");

    /* P(-1) = 1 - 2 + 3 = 2 */
    CHECK(my_iszero(wst_poly_eval(&p, -1.0) - 2.0), "eval at -1");

    /* P(2) = 1 + 4 + 12 = 17 */
    CHECK(my_iszero(wst_poly_eval(&p, 2.0) - 17.0), "eval at 2");
}

static void test_print(void)
{
    WstPoly p;
    char buf[256];

    /* raw mode */
    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_print(buf, ARR_LEN(buf), "P", &p, false);
    CHECK(strstr(buf, "1") && strstr(buf, "2") && strstr(buf, "3"), "print raw contains coeffs");

    /* pretty mode */
    p = (WstPoly){ .degree = 2, .coeffs = { -4, 0, 1 } };
    wst_poly_print(buf, ARR_LEN(buf), "P", &p, true);
    CHECK(strstr(buf, "P(x)"), "print pretty has name");

    /* zero polynomial */
    p = (WstPoly){ .degree = 0, .coeffs = { 0 } };
    wst_poly_print(buf, ARR_LEN(buf), "Q", &p, true);
    CHECK(strstr(buf, "0"), "print zero poly");

    /* truncation: tiny buffer should not crash */
    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_print(buf, 3, "P", &p, true);
}

int main(void)
{
    test_solve();
    test_trim();
    test_scale();
    test_add();
    test_sub();
    test_mul();
    test_cmp();
    test_deriv();
    test_integ();
    test_eval();
    test_print();
    return tests_summary();
}
