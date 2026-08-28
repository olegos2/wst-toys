#ifndef TOYS_POLY_H
#define TOYS_POLY_H

#include <stdbool.h>
#include <stddef.h>

/** Max polynomial degree the solver accepts. */
#define WST_POLY_MAX_DEGREE 16

/** Polynomial with fixed capacity. */
typedef struct {
    /** Coefficients of polynomial ascending power, coeffs[i] matches x^i coeff. */
    double coeffs[WST_POLY_MAX_DEGREE + 1];
    /** Degree of polynomial. */
    int degree;
} WstPoly;

/** Solution for polynomial. */
typedef struct {
    /** Roots, indices beyond `count - 1` should not be used. */
    double roots[WST_POLY_MAX_DEGREE];
    /** Number of distinct values roots array holds. */
    int count;
} WstSolution;

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
 * A degree above WST_POLY_MAX_DEGREE yields WST_SOLVE_ERR.
 *
 * @param [in] poly polynomial to solve
 */
WstSolution wst_poly_solve(WstPoly *poly);

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
 *
 * @param [in] name name for polynomial (e.g. `P` or `y`) in pretty format.
 * @param [in] pretty whether to use pretty formatting or just raw coeffs separated by spaces.
 */
void wst_poly_print(char *buf, size_t nbuf, const char *name,
                    const WstPoly *poly, bool pretty);

#endif /* TOYS_POLY_H */