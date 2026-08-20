#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H

#include <stddef.h>


/** Max polynomial degree the solver accepts. */
#define TOYS_POLY_MAX_DEGREE 2

/** Returned when any real x solves the equation. */
#define TOYS_SOLVE_INF (-1)

/** Returned on invalid args or other error. */
#define TOYS_SOLVE_ERR (-2)

/** Polynomial with fixed capacity, coeff of x^i at coeffs[i]. */
typedef struct {
    double coeffs[TOYS_POLY_MAX_DEGREE + 1];
    /* Actual degree may be lower than full capacity (trailing zeros) */
    int degree;
} ToysPoly;

/**
 * Solves a*x^2 + b*x + c = 0 for real roots. When solutions exist,
 * both x1 and x2 are set, sorted such that x1 <= x2.
 * Returns the number of distinct real roots, 0 if none,
 * TOYS_SOLVE_INF if any real x solves the equation.
 */
int toys_quad_solve(double a, double b, double c, double *x1, double *x2);

/**
 * Solves sum_{i=0..degree} coeffs[i] * x^i = 0 for real roots.
 *
 * Only constant, linear and quadratic equations are accepted. Trailing zero
 * coefficients are ignored. An all-zero polynomial yields TOYS_SOLVE_INF.
 * A degree above TOYS_POLY_MAX_DEGREE yields TOYS_SOLVE_ERR.
 *
 * roots must have room for max_roots entries.
 */
int toys_poly_solve(const ToysPoly *poly, double *roots, int max_roots);

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

#endif /* TOYS_SOLVE_H */