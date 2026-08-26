#include "toys/solve.h"
#include "toys/math.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

static bool poly_eq(const WstPoly *a, const WstPoly *b)
{
    if (a->degree != b->degree)
        return false;
    for (int i = 0; i <= a->degree; i++)
        if (!my_iszero(a->coeffs[i] - b->coeffs[i]))
            return false;
    return true;
}

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
    for (int i = 0; i <= poly->degree; i++)
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

static void test_solve(void)
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
            .poly = { .degree = 3, .coeffs = { 1, 2, 3, 4 } },
            .sol = { .count = WST_SOLVE_ERR },
        },
        /* degree 0 constant zero */
        {
            .poly = { .degree = 0, .coeffs = { 0 } },
            .sol = { .count = WST_SOLVE_INF },
        },
        /* linear: x + 1 = 0 => root -1 */
        {
            .poly = { .degree = 1, .coeffs = { 1, 1 } },
            .sol = { .count = 1, .roots = { -1 } },
        },
        /* quadratic with two irrational roots: x^2 - 2 = 0 */
        {
            .poly = { .degree = 2, .coeffs = { -2, 0, 1 } },
            .sol = { .count = 2, .roots = { -sqrt(2), sqrt(2) } },
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++)
        check_poly(&cases[i].poly, &cases[i].sol);
}

static void test_trim(void)
{
    WstPoly p;

    p = (WstPoly){ .degree = 3, .coeffs = { 1, 2, 0, 0 } };
    wst_poly_trim(&p);
    CHECK(p.degree == 1, "trim trailing zeros");

    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_trim(&p);
    CHECK(p.degree == 2, "trim no trailing zeros");

    p = (WstPoly){ .degree = 0, .coeffs = { 5 } };
    wst_poly_trim(&p);
    CHECK(p.degree == 0, "trim constant");

    p = (WstPoly){ .degree = 4, .coeffs = { 0, 0, 0, 0, 0 } };
    wst_poly_trim(&p);
    CHECK(p.degree == 0, "trim all zeros to degree 0");
}

static void test_scale(void)
{
    WstPoly p;

    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_scale(&p, 2.0);
    CHECK(poly_eq(&p, &(WstPoly){ .degree = 2, .coeffs = { 2, 4, 6 } }), "scale by 2");

    p = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_scale(&p, 0.0);
    CHECK(p.degree == 0 && my_iszero(p.coeffs[0]), "scale by zero");

    p = (WstPoly){ .degree = 2, .coeffs = { 2, 4, 6 } };
    wst_poly_scale(&p, -0.5);
    CHECK(poly_eq(&p, &(WstPoly){ .degree = 2, .coeffs = { -1, -2, -3 } }), "scale by negative");
}

static void test_add(void)
{
    WstPoly a;

    a = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_add(&a, &(WstPoly){ .degree = 2, .coeffs = { 4, 5, 6 } });
    CHECK(poly_eq(&a, &(WstPoly){ .degree = 2, .coeffs = { 5, 7, 9 } }), "add same degree");

    a = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    wst_poly_add(&a, &(WstPoly){ .degree = 3, .coeffs = { 0, 0, 0, 1 } });
    CHECK(poly_eq(&a, &(WstPoly){ .degree = 3, .coeffs = { 1, 2, 0, 1 } }), "add increasing degree");

    a = (WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } };
    wst_poly_add(&a, &(WstPoly){ .degree = 2, .coeffs = { -1, -2, -3 } });
    CHECK(a.degree == 0 && my_iszero(a.coeffs[0]), "add cancels to zero");
}

static void test_sub(void)
{
    WstPoly a;

    a = (WstPoly){ .degree = 2, .coeffs = { 5, 7, 9 } };
    wst_poly_sub(&a, &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } });
    CHECK(poly_eq(&a, &(WstPoly){ .degree = 2, .coeffs = { 4, 5, 6 } }), "sub same degree");

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
    CHECK(poly_eq(&a, &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 1 } }), "mul (1+x)^2");

    /* 3 * x = 3x */
    a = (WstPoly){ .degree = 0, .coeffs = { 3 } };
    CHECK(wst_poly_mul(&a, &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } }), "mul const*x");
    CHECK(poly_eq(&a, &(WstPoly){ .degree = 1, .coeffs = { 0, 3 } }), "mul const*x result");

    /* mul overflow: degree 5 * degree 5 = degree 10 > MAX */
    a = (WstPoly){ .degree = 5, .coeffs = { 1, 1, 1, 1, 1, 1 } };
    CHECK(!wst_poly_mul(&a, &(WstPoly){ .degree = 5, .coeffs = { 1, 1, 1, 1, 1, 1 } }), "mul overflow");
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
    d = wst_poly_deriv(&p);
    CHECK(poly_eq(&d, &(WstPoly){ .degree = 1, .coeffs = { 2, 2 } }), "deriv quadratic");

    /* deriv of constant => 0 */
    p = (WstPoly){ .degree = 0, .coeffs = { 5 } };
    d = wst_poly_deriv(&p);
    CHECK(d.degree == 0 && my_iszero(d.coeffs[0]), "deriv constant");

    /* deriv of 3x^3 + x => 9x^2 + 1 */
    p = (WstPoly){ .degree = 3, .coeffs = { 0, 1, 0, 3 } };
    d = wst_poly_deriv(&p);
    CHECK(poly_eq(&d, &(WstPoly){ .degree = 2, .coeffs = { 1, 0, 9 } }), "deriv cubic");
}

static void test_integ(void)
{
    WstPoly p, out;

    /* integ of 2x + 1 => x^2 + x + C (C=0) */
    p = (WstPoly){ .degree = 1, .coeffs = { 1, 2 } };
    CHECK(wst_poly_integ(&p, &out), "integ succeeds");
    CHECK(poly_eq(&out, &(WstPoly){ .degree = 2, .coeffs = { 0, 1, 1 } }), "integ linear");

    /* integ of constant 5 => 5x */
    p = (WstPoly){ .degree = 0, .coeffs = { 5 } };
    CHECK(wst_poly_integ(&p, &out), "integ constant");
    CHECK(poly_eq(&out, &(WstPoly){ .degree = 1, .coeffs = { 0, 5 } }), "integ constant result");

    /* integ overflow: degree MAX-1 => degree MAX succeeds, degree MAX => fail */
    p = (WstPoly){ .degree = WST_SOLVE_MAX_DEGREE - 1, .coeffs = { 1 } };
    CHECK(wst_poly_integ(&p, &out), "integ at max-1");
    CHECK(out.degree == WST_SOLVE_MAX_DEGREE, "integ degree at max");

    p = (WstPoly){ .degree = WST_SOLVE_MAX_DEGREE, .coeffs = { 1 } };
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
    wst_poly_print(buf, sizeof(buf), "P", &p, false);
    CHECK(strstr(buf, "1") && strstr(buf, "2") && strstr(buf, "3"), "print raw contains coeffs");

    /* pretty mode */
    p = (WstPoly){ .degree = 2, .coeffs = { -4, 0, 1 } };
    wst_poly_print(buf, sizeof(buf), "P", &p, true);
    CHECK(strstr(buf, "P(x)"), "print pretty has name");

    /* zero polynomial */
    p = (WstPoly){ .degree = 0, .coeffs = { 0 } };
    wst_poly_print(buf, sizeof(buf), "Q", &p, true);
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
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
