#ifndef TOYS_QUAD_H
#define TOYS_QUAD_H

/**
 * Result of solving an equation of form a*x^2 + b*x + c = 0.
 */
typedef enum {
    TOYS_QUAD_TWO_ROOTS,   /**< two real roots, x1 != x2 */
    TOYS_QUAD_ONE_ROOT,    /**< one real root, x1 == x2 */
    TOYS_QUAD_NO_ROOTS,    /**< no real roots */
    TOYS_QUAD_DEGENERATE,  /**< a == 0: linear equation, single root in x1 */
    TOYS_QUAD_INVALID,     /**< a == 0, b == 0: every x (c == 0) or no x (c != 0) is a root */
} ToysQuadResult;

/**
 * Solve quadratic equation a*x^2 + b*x + c = 0 for real roots.
 *
 * On success (TWO_ROOTS/ONE_ROOT/DEGENERATE) stores the roots in x1 and x2.
 */
ToysQuadResult toys_quad_solve(double a, double b, double c, double *x1, double *x2);

#endif /* TOYS_QUAD_H */