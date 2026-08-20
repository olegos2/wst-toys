#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H


/** Max polynomial degree the solver accepts. */
#define TOYS_POLY_MAX_DEGREE 64

/* Common result convention for toys_*_solve functions. */

/** Any real x solves the equation. */
#define TOYS_SOLVE_INF (-1)

/** Invalid args or other error. */
#define TOYS_SOLVE_ERR (-2)

/** Polynomial with fixed capacity, coeff of x^i at coeffs[i]. */
typedef struct {
    double coeffs[TOYS_POLY_MAX_DEGREE + 1];
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
 * Trailing zero coefficients (coeffs[degree] == 0) are ignored; an
 * all-zero polynomial yields TOYS_SOLVE_INF. For degree > 2 the roots
 * of P' split the real line (Rolle's theorem) into monotone intervals;
 * sign changes are caught by bisection and refined with Newton,
 * even-multiplicity roots are found at critical points.
 *
 * Returns the number of distinct real roots, filled into roots in
 * ascending order, or a negative TOYS_SOLVE_* constant.
 *
 * roots must have room for max_roots entries.
 */
int toys_poly_solve(const ToysPoly *poly, double *roots, int max_roots);

#endif /* TOYS_SOLVE_H */