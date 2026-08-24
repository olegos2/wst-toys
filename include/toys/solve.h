#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H

#include <stdbool.h>
#include <stddef.h>

/** Max polynomial degree the solver accepts. */
#define WST_SOLVE_MAX_DEGREE 8

enum {
    /** Returned when any real x solves the equation. */
    WST_SOLVE_INF = -1,
    /** Returned on invalid args or other error. */
    WST_SOLVE_ERR = -2,
};

/**
 * Polynomial with fixed capacity.
 */
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
 * Parses the expression in s of length len and reduces it to a
 * polynomial, with + - * / ( ) = ^ as operators. A top-level '='
 * turns a = b into a - b = 0.
 *
 * Returns NULL on success and fills in *out. On failure returns a
 * static error message and stores its byte offset in *err_pos.
 */
const char *wst_expr_to_poly(const char *s, size_t len, WstPoly *out,
                             size_t *err_pos);

/** Scales all coeffs of polynomial */
void wst_poly_scale(WstPoly *a, double s);

/** Adds matching coeffs of `b` to `a` and updates degree. */
void wst_poly_add(WstPoly *a, const WstPoly *b);

/** Subtracts matching coeffs of polynomial `b` from `a`. */
void wst_poly_sub(WstPoly *a, const WstPoly *b);

/**
 * Multiplies and sums (convolutes) coefficients of polynomials,
 * returns false if resulting degree would not fit.
 */
bool wst_poly_mul(WstPoly *a, const WstPoly *b);

/** Returns derivative polynomial with degree lowered by one,
 * derivative of a constant is the zero polynomial. */
WstPoly wst_poly_deriv(const WstPoly *poly);

/** Integrate polynomial, returns `false` on error. */
bool wst_poly_integ(const WstPoly *poly, WstPoly *out);

/** Evaluate polynomial at a point x. */
double wst_poly_eval(const WstPoly *poly, double x);

#endif /* TOYS_SOLVE_H */