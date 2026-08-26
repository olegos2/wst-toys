#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H

#include <stdbool.h>
#include <stddef.h>

/** Max polynomial degree the solver accepts. */
#define WST_SOLVE_MAX_DEGREE 8

/** Polynomial with fixed capacity. */
typedef struct {
    /** Coefficients of polynomial ascending power, coeffs[i] matches x^i coeff. */
    double coeffs[WST_SOLVE_MAX_DEGREE + 1];
    /** Degree of polynomial. */
    int degree;
} WstPoly;

/** Solution for polynomial. */
typedef struct {
    /** Roots, indices beyond `count - 1` should not be used. */
    double roots[WST_SOLVE_MAX_DEGREE];
    /** Number of distinct values roots array holds. */
    int count;
} WstSolution;

/** Error that parser may return. */
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

enum {
    /** Returned when any real x solves the equation. */
    WST_SOLVE_INF = -1,
    /** Returned on invalid args or other error. */
    WST_SOLVE_ERR = -2,
};

/** Drop trailing zero coeffs if any (never increases degree). */
void wst_poly_trim(WstPoly *p);

/**
 * Solves sum_{i=0..degree} coeffs[i] * x^i = 0 for real roots.
 *
 * Only constant, linear and quadratic equations are accepted. Trailing zero
 * coefficients are ignored. An all-zero polynomial yields WST_SOLVE_INF.
 * A degree above WST_SOLVE_MAX_DEGREE yields WST_SOLVE_ERR.
 *
 * @param [in] poly polynomial to solve
 */
WstSolution wst_solve_poly(WstPoly *poly);

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
 */
WstParserErr wst_expr_to_poly(const char *s, WstPoly *out, size_t *err_pos, bool expr_mode);

/** Get error string for parser error number. */
const char *wst_expr_err_string(WstParserErr err);

/** Scale all coeffs of polynomial by number. */
void wst_poly_scale(WstPoly *a, double s);

/** Add matching coeffs of `b` to `a` and updates degree. */
void wst_poly_add(WstPoly *a, const WstPoly *b);

/** Subtract matching coeffs of polynomial `b` from `a`. */
void wst_poly_sub(WstPoly *a, const WstPoly *b);

/** Compare two polynomials equal: degree, then every coefficient. */
bool wst_poly_cmp(const WstPoly *a, const WstPoly *b);

/** Convolute (multiply) `a` by `b`, returns `false` on error. */
bool wst_poly_mul(WstPoly *a, const WstPoly *b);

/** Get derivative of polynomial. */
WstPoly wst_poly_deriv(const WstPoly *poly);

/** Get indefinite integral of polynomial, returns `false` on error. */
bool wst_poly_integ(const WstPoly *poly, WstPoly *out);

/** Evaluate polynomial at a point x. */
double wst_poly_eval(const WstPoly *poly, double x);

/**
 * Print a polynomial into buffer of length `nbuf`.
 * @param [in] name name for polynomial (e.g. `P` or `y`) in pretty format.
 * @param [in] pretty whether to use pretty formatting or just raw coeffs separated by spaces.
 */
void wst_poly_print(char *buf, size_t nbuf, const char *name,
                    const WstPoly *poly, bool pretty);

#endif /* TOYS_SOLVE_H */