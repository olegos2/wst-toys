#include "toys/poly.h"
#include "toys/expr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        fprintf(stderr, "\nFAIL (test %d) expression error at position %zu: %s\n"
                "     input: %s\n",
                counter, err_pos, wst_expr_err_string(ret), expr);
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
            "     got wrong error code: %s (pos %zu)\n     expected error: %s\n",
            counter, expr, wst_expr_err_string(ret), err_pos, wst_expr_err_string(exp_err));
    failures++;
}

static void test_coeff(const char *input, const WstPoly *exp_poly)
{
    assert(input != NULL);
    assert(exp_poly != NULL);

    counter++;

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(input, &res, &err_pos, false);
    if (ret != WST_EXPR_NO_ERR) {
        fprintf(stderr, "\nFAIL (test %d) coeff parse error at %zu: %s\n"
                "     input: %s\n",
                counter, err_pos, wst_expr_err_string(ret), input);
        failures++;
    } else if (!wst_poly_cmp(&res, exp_poly)) {
        fprintf(stderr, "\nFAIL (test %d) coeff input: %s\n", counter, input);
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
    counter++;

    WstPoly res = { 0 };
    size_t err_pos = 0;
    WstParserErr ret = wst_expr_to_poly(input, &res, &err_pos, false);
    if (ret == exp_err)
        return;

    fprintf(stderr, "\nFAIL (test %d) coeff input: %s\n"
            "     got error: %s, expected: %s\n",
            counter, input, wst_expr_err_string(ret), wst_expr_err_string(exp_err));
    failures++;
}

typedef struct {
    const char *expr;
    WstParserErr exp_ret;
    WstPoly exp_poly;
} ExprTestCase;

int main(void)
{
    test_expr("0", &(WstPoly){ .degree = 0, .coeffs = { 0 } });
    test_expr("42", &(WstPoly){ .degree = 0, .coeffs = { 42 } });
    test_expr("3.14", &(WstPoly){ .degree = 0, .coeffs = { 3.14 } });
    test_expr("0.4e+2", &(WstPoly){ .degree = 0, .coeffs = { 40.0 } });
    test_expr(".5", &(WstPoly){ .degree = 0, .coeffs = { 0.5 } });

    test_expr("x", &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } });
    test_expr("  x  ", &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } });

    test_expr("1 + 2", &(WstPoly){ .degree = 0, .coeffs = { 3 } });
    test_expr("x + 1", &(WstPoly){ .degree = 1, .coeffs = { 1, 1 } });
    test_expr("x + x", &(WstPoly){ .degree = 1, .coeffs = { 0, 2 } });

    test_expr("5 - 3", &(WstPoly){ .degree = 0, .coeffs = { 2 } });
    test_expr("x - 1", &(WstPoly){ .degree = 1, .coeffs = { -1, 1 } });
    test_expr("1 - x", &(WstPoly){ .degree = 1, .coeffs = { 1, -1 } });

    test_expr("3 * 4", &(WstPoly){ .degree = 0, .coeffs = { 12 } });
    test_expr("3 * x", &(WstPoly){ .degree = 1, .coeffs = { 0, 3 } });
    test_expr("x * x", &(WstPoly){ .degree = 2, .coeffs = { 0, 0, 1 } });
    test_expr("(x + 1) * (x - 1)", &(WstPoly){ .degree = 2, .coeffs = { -1, 0, 1 } });

    test_expr("x / 2", &(WstPoly){ .degree = 1, .coeffs = { 0, 0.5 } });
    test_expr("6 / 3", &(WstPoly){ .degree = 0, .coeffs = { 2 } });

    test_expr("x ^ 0", &(WstPoly){ .degree = 0, .coeffs = { 1 } });
    test_expr("x ^ 1", &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } });
    test_expr("x ^ 2", &(WstPoly){ .degree = 2, .coeffs = { 0, 0, 1 } });
    test_expr("x ^ 3", &(WstPoly){ .degree = 3, .coeffs = { 0, 0, 0, 1 } });
    test_expr("2 ^ 3", &(WstPoly){ .degree = 0, .coeffs = { 8 } });

    test_expr("-5", &(WstPoly){ .degree = 0, .coeffs = { -5 } });
    test_expr("+5", &(WstPoly){ .degree = 0, .coeffs = { 5 } });
    test_expr("--5", &(WstPoly){ .degree = 0, .coeffs = { 5 } });
    test_expr("-x", &(WstPoly){ .degree = 1, .coeffs = { 0, -1 } });
    test_expr("---x", &(WstPoly){ .degree = 1, .coeffs = { 0, -1 } });

    test_expr("(1)", &(WstPoly){ .degree = 0, .coeffs = { 1 } });
    test_expr("((x))", &(WstPoly){ .degree = 1, .coeffs = { 0, 1 } });
    test_expr("(x + 1) * (x - 1)", &(WstPoly){ .degree = 2, .coeffs = { -1, 0, 1 } });

    test_expr("x^2 - 5*x + 6", &(WstPoly){ .degree = 2, .coeffs = { 6, -5, 1 } });

    test_expr("(x + 1) ^ 2", &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 1 } });

    test_expr("2 + 3 * 4", &(WstPoly){ .degree = 0, .coeffs = { 14 } });

    test_expr("((x+1)*(x-1))^1", &(WstPoly){ .degree = 2, .coeffs = { -1, 0, 1 } });

    test_expr("( 0 - 1.5) * 2 + (x + .5) ^ 2", &(WstPoly){ .degree = 2, .coeffs = { -2.75, 1.0, 1.0 } });

    test_expr_err("", WST_EXPR_UNEXPECTED_END_OF_EXPR);
    test_expr_err("  ", WST_EXPR_UNEXPECTED_END_OF_EXPR);
    test_expr_err("x +", WST_EXPR_UNEXPECTED_END_OF_EXPR);
    test_expr_err("* 5", WST_EXPR_UNEXPECTED_ATOM);
    test_expr_err("5 + * 3", WST_EXPR_UNEXPECTED_ATOM);
    test_expr_err("(1 + 2", WST_EXPR_MISSING_RPAREN);
    test_expr_err("1 + 2)", WST_EXPR_UNEXPECTED_ATOM);
    test_expr_err("1 @ 2", WST_EXPR_UNEXPECTED_CHAR_IN_EXPR);
    test_expr_err("x ^ -1", WST_EXPR_NON_INTEGER_POWER);
    test_expr_err("x ^ 1.5", WST_EXPR_NON_INTEGER_POWER);
    test_expr_err("x ^ x", WST_EXPR_NON_INTEGER_POWER);
    test_expr_err("1 / 0", WST_EXPR_DIV_ERR);
    test_expr_err("1 / x", WST_EXPR_DIV_ERR);
    test_expr_err("5 / (x + 1)", WST_EXPR_DIV_ERR);
    test_expr_err("1 / (x - x)", WST_EXPR_DIV_ERR);
    test_expr_err("0.4e+2 @", WST_EXPR_UNEXPECTED_CHAR_IN_EXPR);

    test_coeff("1", &(WstPoly){ .degree = 0, .coeffs = { 1 } });
    test_coeff("1 2 3", &(WstPoly){ .degree = 2, .coeffs = { 1, 2, 3 } });
    test_coeff("  6  -5  1  ", &(WstPoly){ .degree = 2, .coeffs = { 6, -5, 1 } });
    test_coeff("0 0 1", &(WstPoly){ .degree = 2, .coeffs = { 0, 0, 1 } });
    test_coeff("5 0", &(WstPoly){ .degree = 1, .coeffs = { 5, 0 } });

    test_coeff_err("", WST_EXPR_UNEXPECTED_END_OF_EXPR);
    test_coeff_err("abc", WST_EXPR_FAILED_TO_PARSE_NUMBER);

    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
