#include "toys/debug.h"
#include "toys/solve.h"
#include "poly_impl.h"

#include <assert.h>
#include <math.h>


int toys_quad_solve(double a, double b, double c, double *x1, double *x2)
{
    assert(x1 != NULL && x2 != NULL && x1 != x2);

    LOG_V("a=%g b=%g c=%g", a, b, c);

    if (a == 0.0) {
        if (b == 0.0)
            return (c == 0.0) ? TOYS_SOLVE_INF : 0;
        *x1 = -c / b;
        *x2 = *x1;
        return 1;
    }

    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return 0;

    double sqrt_disc = sqrt(disc);
    if (sqrt_disc == 0.0) {
        *x1 = -b / (2.0 * a);
        *x2 = *x1;
        return 1;
    }

    /* sorted output: x1 <= x2 */
    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);
    if (a < 0.0) {
        *x1 = t2;
        *x2 = t1;
    } else {
        *x1 = t1;
        *x2 = t2;
    }
    return 2;
}


int toys_poly_solve(const ToysPoly *poly, double *roots, int max_roots)
{
    assert(poly != NULL && roots != NULL);

    if (poly->degree < 0 || max_roots < 0) {
        LOG_E("invalid args: degree=%d, max_roots=%d", poly->degree, max_roots);
        return TOYS_SOLVE_ERR;
    }
    if (poly->degree > TOYS_POLY_MAX_DEGREE) {
        LOG_E("degree %d exceeds TOYS_POLY_MAX_DEGREE %d",
              poly->degree, TOYS_POLY_MAX_DEGREE);
        return TOYS_SOLVE_ERR;
    }

    ToysPoly p = *poly;
    toys_poly_trim(&p);

    /* constant: every x is a solution, or none */
    if (p.degree == 0) {
        LOG_D("constant equation: %s",
              p.coeffs[0] == 0.0 ? "every x is a solution" : "no solutions");
        return (p.coeffs[0] == 0.0) ? TOYS_SOLVE_INF : 0;
    }

    /* a*x + b = 0 */
    if (p.degree == 1) {
        LOG_D("linear equation");
        if (max_roots > 0)
            roots[0] = -p.coeffs[0] / p.coeffs[1];
        return 1;
    }

    /* exact quadratic formula */
    LOG_D("quadratic equation");
    double x1, x2;
    int count = toys_quad_solve(p.coeffs[2], p.coeffs[1], p.coeffs[0], &x1, &x2);
    if (count <= 0)
        return count;
    if (max_roots > 0)
        roots[0] = x1;
    if (count == 2 && max_roots > 1)
        roots[1] = x2;
    return count;
}