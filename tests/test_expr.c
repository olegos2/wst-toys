#include "toys/solve.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

static int failures = 0;
static int counter = 0;

static void test_expr(const char *expr, const WstPoly *exp_poly)
{
    assert(expr != NULL);
    assert(exp_poly != NULL);

    counter++;

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(expr, &res, &err_pos, true);
    if (ret != WST_EXPR_NO_ERR) {
        fprintf(stderr, "\nFAIL (test %d) expression error at position %zu: %s\n",
                counter, err_pos, wst_expr_err_string(ret));
        failures++;
    } else if (!wst_poly_cmp(&res, exp_poly)) {
        fprintf(stderr, "\nFAIL (test %d) input expr: %s\n", counter, expr);
        fprintf(stderr, "     expected coeffs (degree %d): ", exp_poly->degree);
        for (int i = 0; i <= exp_poly->degree; i++)
            fprintf(stderr, "%lg ", exp_poly->coeffs[i]);
        fprintf(stderr, "\n     got coeffs (degree %d): ", res.degree);
        for (int i = 0; i <= res.degree; i++)
            fprintf(stderr, "%lg ", res.coeffs[i]);
        fprintf(stderr, "\n");
        failures++;
    }
}

static void test_expr_err(const char *expr, WstParserErr exp_err)
{
    assert(expr != NULL);
    assert(exp_err != WST_EXPR_NO_ERR);

    counter++;

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(expr, &res, &err_pos, true);
    if (ret == exp_err)
        return;

    fprintf(stderr, "\nFAIL (test %d) input expr: %s\n"
            "     got wrong error code: %s\n     expected error: %s\n",
            counter, expr, wst_expr_err_string(ret), wst_expr_err_string(exp_err));
    failures++;
}

typedef struct {
    const char *expr;
    WstParserErr exp_ret;
    WstPoly exp_poly;
} TestCase;

int main(void)
{
    static TestCase cases[] = {
        {
            .expr = "",
            .exp_ret = WST_EXPR_UNEXPECTED_END_OF_EXPR,
        },
        {
            .expr = "0.4e+2",
            .exp_ret = WST_EXPR_NO_ERR,
            .exp_poly = { .degree = 0, .coeffs = { 0 } },
        },
        {
            .expr = "( 0 - 1.5) * 2 + (x + .5) ^ 2",
            .exp_ret = WST_EXPR_NO_ERR,
            .exp_poly = { .degree = 2, .coeffs = { -2.75, 1.0, 1.0 } },
        },
        // TODO
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        if (cases[i].exp_ret == WST_EXPR_NO_ERR)
            test_expr(cases[i].expr, &cases[i].exp_poly);
        else
            test_expr_err(cases[i].expr, cases[i].exp_ret);
    }

    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 0;
}
