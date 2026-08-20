#ifndef TOYS_SOLVE_H
#define TOYS_SOLVE_H

/** Max polynomial degree the solver accepts. */
#define TOYS_POLY_MAX_DEGREE 64

/**
 * Common result convention for toys_*_solve functions.
 */

/** Infinitely many solutions */
#define TOYS_SOLVE_INF (-1)

/** Invalid args or other error */
#define TOYS_SOLVE_ERR (-2)

/**
 * Solve the quadratic equation a*x^2 + b*x + c = 0 for real roots.
 *
 * When solutions exist, both x1 and x2 are set, sorted (`x1 <= x2`).
 *
 * @param [in] a quadratic coeff
 * @param [in] b linear coeff
 * @param [in] c constant coeff
 * @param [out] x1 first sol
 * @param [out] x2 second sol
 *
 * @return number of distinct real roots, 0 if none, `TOYS_SOLVE_INF` if any
 *         real x is a solution.
 */
int toys_quad_solve(double a, double b, double c, double *x1, double *x2);

/**
 * Solve `sum(coeffs[i] * x^i, i = 0..degree) = 0` for real roots.
 *
 * High-degree zero coefficients are ignored; an all-zero polynomial
 * has every real x as a solution and yields `TOYS_SOLVE_INF`. For
 * degree > 2 the roots of P' (Rolle's theorem) split the real line
 * into monotone intervals; sign changes there are caught by bisection
 * and refined with Newton, even-multiplicity roots are found at
 * critical points.
 *
 * Returns the number of distinct real roots, or a negative `TOYS_SOLVE_*`
 * constant.
 *
 * @param [in] degree polynomial degree
 * @param [in] coeffs `degree + 1` coefficients, ascending powers of x
 * @param [out] roots solutions
 * @param [in] max_roots output capacity, should be `degree + 1`
 */
int toys_poly_solve(int degree, const double *coeffs, double *roots, int max_roots);

#endif /* TOYS_SOLVE_H */