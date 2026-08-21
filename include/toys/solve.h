#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H

#include <stddef.h>
#include <stdbool.h>
#include <float.h>

/** Max polynomial degree the solver accepts. */
#define TOYS_POLY_MAX_DEGREE 2

enum {
    /** Returned when any real x solves the equation. */
    TOYS_SOLVE_INF = -1,
    /** Returned on invalid args or other error. */
    TOYS_SOLVE_ERR = -2,
};

/**
 * Polynomial with fixed capacity.
 */
typedef struct {
    /** Coefficients of polynomial ascending power, coeffs[i] matches x^i coeff. */
    double coeffs[TOYS_POLY_MAX_DEGREE + 1];
    /** Degree of polynomial. */
    int degree;
} ToysPoly;

/** Solution for polynomial. */
typedef struct {
    /** Roots, indices beyond `count - 1` should not be used. */
    double roots[TOYS_POLY_MAX_DEGREE];
    /** Number of distinct values roots array holds. */
    int count;
} ToysSolution;

/** Drop trailing zero coeffs if any (never increases degree). */
void toys_poly_trim(ToysPoly *p);

/**
 * Solves sum_{i=0..degree} coeffs[i] * x^i = 0 for real roots.
 *
 * Only constant, linear and quadratic equations are accepted. Trailing zero
 * coefficients are ignored. An all-zero polynomial yields TOYS_SOLVE_INF.
 * A degree above TOYS_POLY_MAX_DEGREE yields TOYS_SOLVE_ERR.
 *
 * @param [in] poly polynomial to solve
 */
ToysSolution toys_poly_solve(ToysPoly *poly);

/**
 * Parses the expression in s of length len and reduces it to a
 * polynomial, with + - * / ( ) = ^ as operators. A top-level '='
 * turns a = b into a - b = 0.
 *
 * Returns NULL on success and fills in *out. On failure returns a
 * static error message and stores its byte offset in *err_pos.
 */
const char *toys_expr_to_poly(const char *s, size_t len, ToysPoly *out,
                              size_t *err_pos);

/** Scales all coeffs of polynomial */
ToysPoly toys_poly_scale(const ToysPoly *a, double s);

/** Sums each coeff of 2 polynomials, result has correct degree set. */
ToysPoly toys_poly_add(const ToysPoly *a, const ToysPoly *b);

/** Subtracts matching coeffs of polynomial `b` from `a`. */
ToysPoly toys_poly_sub(const ToysPoly *a, const ToysPoly *b);

/**
 * Multiplies and sums (convolutes) coefficients of polynomials,
 * returns false if resulting degree would not fit.
 */
bool toys_poly_mul(const ToysPoly *a, const ToysPoly *b, ToysPoly *out);

/** Check if `double` is in `-DBL_EPSILON..DBL_EPSILON` range. */
static inline bool iszero(double a)
{
    return a > -DBL_EPSILON && a < DBL_EPSILON;
}

#endif /* TOYS_SOLVE_H */