#include "tests_common.h"

#include "toys/poly.h"
#include "toys/expr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_coeff(const char *input, const WstPoly *exp_poly)
{
    assert(input != NULL);
    assert(exp_poly != NULL);

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(input, &res, &err_pos, false);
    if (ret != WST_EXPR_NO_ERR) {
        fprintf(stderr, "\nFAIL coeff parse error at %zu: %s\n"
                "     input: %s\n", err_pos, wst_expr_err_string(ret), input);
        failures++;
    } else if (!wst_poly_cmp(&res, exp_poly)) {
        fprintf(stderr, "\nFAIL coeff input: %s\n", input);
        fprintf(stderr, "     expected (degree %d): ", exp_poly->degree);
        for (int i = 0; i <= exp_poly->degree; i++)
            fprintf(stderr, "%lg ", exp_poly->coeffs[i]);
        fprintf(stderr, "\n     got (degree %d): ", res.degree);
        for (int i = 0; i <= res.degree; i++)
            fprintf(stderr, "%lg ", res.coeffs[i]);
        fprintf(stderr, "\n");
        failures++;
    }
}

static void test_coeff_err(const char *input, WstParserErr exp_err)
{
    assert(input != NULL);

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(input, &res, &err_pos, false);
    if (ret == exp_err)
        return;

    fprintf(stderr, "\nFAIL coeff input: %s\n"
            "     got error: %s, expected: %s\n",
            input, wst_expr_err_string(ret), wst_expr_err_string(exp_err));
    failures++;
}

int main(void)
{
    assert(WST_EXPR_NO_ERR == 0);

    /* TODO: Finish this refactor. */

    struct {
        const char *expr;
        WstParserErr exp_ret;
        WstPoly exp_poly;
    } cases[] = {
        {
            .expr = "0",
            .exp_poly = { .degree = 0, .coeffs = { 0 } },
        },
        {
            .expr = "42.",
            .exp_poly = { .degree = 0, .coeffs = { 42 } },
        },
        {
            .expr = "3.14",
            .exp_poly = { .degree = 0, .coeffs = { 3.14 } },
        },
        {
            .expr = "0.4e+2",
            .exp_poly = { .degree = 0, .coeffs = { 0.4e+2 } },
        },
        {
            .expr = ".5",
            .exp_poly = { .degree = 0, .coeffs = { .5 } },
        },
        {
            .expr = "x",
            .exp_poly = { .degree = 1, .coeffs = { 0, 1 } },
        },
        {
            .expr = "x + 1",
            .exp_poly = { .degree = 1, .coeffs = { 1, 1 } },
        },
        {
            .expr = " ( x +1 + 3+ x)  ",
            .exp_poly = { .degree = 1, .coeffs = { 4, 2 } },
        },
        {
            .expr = "x / 2",
            .exp_poly = { .degree = 1, .coeffs = { 0, 0.5 } },
        },
        {
            .expr = "6 / 3",
            .exp_poly = { .degree = 0, .coeffs = { 2 } },
        },
        {
            .expr = "x ^ 0",
            .exp_poly = { .degree = 0, .coeffs = { 1 } },
        },
        {
            .expr = "x ^ 1",
            .exp_poly = { .degree = 1, .coeffs = { 0, 1 } },
        },
        {
            .expr = "x ^ 2",
            .exp_poly = { .degree = 2, .coeffs = { 0, 0, 1 } },
        },
        {
            .expr = "x ^ 3",
            .exp_poly = { .degree = 3, .coeffs = { 0, 0, 0, 1 } }
        },
        {
            .expr = "2 ^ 3",
            .exp_poly = { .degree = 0, .coeffs = { 8 } },
        },
        {
            .expr = "x ** 3",
            .exp_poly = { .degree = 3, .coeffs = { 0, 0, 0, 1 } },
        },
        {
            .expr = "2 ** 3",
            .exp_poly = { .degree = 0, .coeffs = { 8 } },
        },
        {
            .expr = "-5",
            .exp_poly = { .degree = 0, .coeffs = { -5 } },
        },
        {
            .expr = "+5",
            .exp_poly = { .degree = 0, .coeffs = { 5 } },
        },
        {
            .expr = "--5",
            .exp_poly = { .degree = 0, .coeffs = { 5 } },
        },
        {
            .expr = "-x",
            .exp_poly = { .degree = 1, .coeffs = { 0, -1 } },
        },
        {
            .expr = "---x",
            .exp_poly = { .degree = 1, .coeffs = { 0, -1 } },
        },
        {
            .expr = "(1)",
            .exp_poly = { .degree = 0, .coeffs = { 1 } },
        },
        {
            .expr = "((x))",
            .exp_poly = { .degree = 1, .coeffs = { 0, 1 } }
        },
        {
            .expr = "(x + 1) * (x - 1)",
            .exp_poly = { .degree = 2, .coeffs = { -1, 0, 1 } }
        },
        {
            .expr = "x^2 - 5*x + 6",
            .exp_poly = { .degree = 2, .coeffs = { 6, -5, 1 } },
        },
        {
            .expr = "(x + 1) ^ 2",
            .exp_poly = { .degree = 2, .coeffs = { 1, 2, 1 } },
        },
        {
            .expr = "2 + 3 * 4",
            .exp_poly = { .degree = 0, .coeffs = { 14 } },
        },
        {
            .expr = "((x+1)*(x-1))^1",
            .exp_poly = { .degree = 2, .coeffs = { -1, 0, 1 } },
        },
        {
            .expr = "( 0 - 1.5) * 2 + (x + .5) ^ 2",
            .exp_poly = { .degree = 2, .coeffs = { -2.75, 1.0, 1.0 } },
        },
        {
            .expr = "", 
            .exp_ret = WST_EXPR_UNEXPECTED_END_OF_EXPR,
        },
        {
            .expr = "  ",
            .exp_ret = WST_EXPR_UNEXPECTED_END_OF_EXPR,
        },
        {
            .expr = "x +",
            .exp_ret = WST_EXPR_UNEXPECTED_END_OF_EXPR,
        },
        {
            .expr = "* 5",
            .exp_ret = WST_EXPR_UNEXPECTED_ATOM,
        },
        {
            .expr = "5 + * 3",
            .exp_ret = WST_EXPR_UNEXPECTED_ATOM,
        },
        {
            .expr = "(1 + 2",
            .exp_ret = WST_EXPR_MISSING_RPAREN,
        },
        {
            .expr = "1 + 2)",
            .exp_ret = WST_EXPR_UNEXPECTED_ATOM,
        },
        {
            .expr = "1 @ 2",
            .exp_ret = WST_EXPR_UNEXPECTED_CHAR_IN_EXPR,
        },
        {
            .expr = "x ^ -1",
            .exp_ret = WST_EXPR_NON_INTEGER_POWER,
        },
        {
            .expr = "x ^ 1.5",
            .exp_ret = WST_EXPR_NON_INTEGER_POWER,
        },
        {
            .expr = "x ^ x",
            .exp_ret = WST_EXPR_NON_INTEGER_POWER,
        },
        {
            .expr = "1 / 0",
            .exp_ret = WST_EXPR_DIV_ERR,
        },
        {
            .expr = "1 / x",
            .exp_ret = WST_EXPR_DIV_ERR,
        },
        {
            .expr = "5 / (x + 1)",
            .exp_ret = WST_EXPR_DIV_ERR,
        },
        {
            .expr = "1 / (x - x)",
            .exp_ret = WST_EXPR_DIV_ERR,
        },
        {
            .expr = "0.4e+2 @",
            .exp_ret = WST_EXPR_UNEXPECTED_CHAR_IN_EXPR,
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        if (cases[i].exp_ret != WST_EXPR_NO_ERR) {
            WstPoly res = { 0 };
            size_t err_pos = 0;
            WstParserErr ret = wst_expr_to_poly(cases[i].expr, &res, &err_pos, true);
            if (ret != cases[i].exp_ret) {
                FAIL("expr #%zu (%s) wrong error code: %s, expected: %s",
                     i, cases[i].expr, wst_expr_err_string(ret), wst_expr_err_string(cases[i].exp_ret));
            }
            continue;
        }
        WstPoly res = { 0 };
        size_t err_pos = 0;
        WstParserErr ret = wst_expr_to_poly(cases[i].expr, &res, &err_pos, true);
        if (ret != WST_EXPR_NO_ERR) {
            FAIL("expr #%zu (%s) error at position %zu: %s",
                 i, cases[i].expr, err_pos, wst_expr_err_string(ret));
        } else if (!wst_poly_cmp(&res, &cases[i].exp_poly)) {
            FAIL("expr #%zu (%s):", i, cases[i].expr);
            print_poly(&res, "Got");
            print_poly(&cases[i].exp_poly, "Expected");
        }
    }

    test_coeff("1", &(WstPoly){ .degree = 0, .coeffs = { 1 } });
    test_coeff("1 2 3", &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } });
    test_coeff("  6  -5  1  ", &(WstPoly){ .degree = 2, .coeffs = { 6, -5, 1 } });
    test_coeff("0 0 1", &(WstPoly){ .degree = 2, .coeffs = { 0, 0, 1 } });
    test_coeff("5 0", &(WstPoly){ .degree = 1, .coeffs = { 5, 0 } });

    test_coeff_err("", WST_EXPR_UNEXPECTED_END_OF_EXPR);
    test_coeff_err("abc", WST_EXPR_FAILED_TO_PARSE_NUMBER);

    return tests_summary();
}
