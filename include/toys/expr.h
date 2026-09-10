#ifndef TOYS_EXPR_H
#define TOYS_EXPR_H

#include "toys/poly.h"

#include <stddef.h>
#include <stdbool.h>

/** Error that parser may return. No error is always `0`. */
typedef enum {
    WST_EXPR_NO_ERR = 0,
    WST_EXPR_FAILED_TO_PARSE_NUMBER,
    WST_EXPR_UNEXPECTED_CHAR_IN_NUM,
    WST_EXPR_UNEXPECTED_CHAR_IN_EXPR,
    WST_EXPR_UNEXPECTED_END_OF_EXPR,
    WST_EXPR_UNEXPECTED_ATOM,
    WST_EXPR_NON_INTEGER_POWER,
    WST_EXPR_DEGREE_EXCEEDED,
    WST_EXPR_DIV_ERR,
    WST_EXPR_MISSING_RPAREN,
} WstParserErr;

/**
 * When `expr_mode` is `true`:
 * parses the expression in s and reduces it to a
 * polynomial, with + - * / ( ) ^ as operators.
 *
 * When `expr_mode` is `false`:
 * parses raw coeff numbers in ascending order separated by spaces from string.
 *
 * Returns `WST_EXPR_NO_ERR` on success and fills in *out. On failure returns an
 * error code and stores error offset in string in `*err_pos`.
 *
 * Expression parsing grammar
 * - num   := [0-9]*\.?[0-9]+
 * - var   := 'x'
 * - prim  := '(' expr ')' | num | var
 * - pow   := prim '^' prim
 * - unary := [-+]* num
 * - term  := unary [* /] unary +
 * - expr  := term [+-] term +
 *
 * @param[in] s String that contains expression.
 * @param[out] out Resulting polynomial.
 * @param[out] err_pos Position in string where parsing error occured.
 * @param[in] expr_mode Whether input string is a math expression or raw polynomial coeffs.
 */
WstParserErr wst_expr_to_poly(const char *s, WstPoly *out, size_t *err_pos, bool expr_mode);

/** Get error string for parser error number. */
const char *wst_expr_err_string(WstParserErr err);

#endif /* TOYS_EXPR_H */
