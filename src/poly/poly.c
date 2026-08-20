#include "toys/debug.h"
#include "toys/solve.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

#include <stdlib.h>

#define ROOT_EPS 1e-6

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

    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);

    /* sorted output x1 <= x2 */
    if (a < 0.0) {
        *x1 = t2;
        *x2 = t1;
    } else {
        *x1 = t1;
        *x2 = t2;
    }
    return 2;
}

/**
 * Horner evaluation of P(x) = coeffs[0] + x*(coeffs[1] + ... + x*coeffs[degree]).
 */
static double poly_eval(const double *coeffs, int degree, double x)
{
    double v = coeffs[degree];
    for (int i = degree - 1; i >= 0; i--)
        v = v * x + coeffs[i];
    return v;
}

/**
 * Horner evaluation of P'(x) = coeffs[1] + 2 coeffs[2] x + ... +
 * degree coeffs[degree] x^(degree-1).
 */
static double poly_eval_deriv(const double *coeffs, int degree, double x)
{
    double v = degree * coeffs[degree];
    for (int i = degree - 1; i >= 1; i--)
        v = v * x + i * coeffs[i];
    return v;
}

/**
 * qsort comparator for doubles; NaN compares equal to everything.
 */
static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa;
    double b = *(const double *)pb;
    return (a > b) - (a < b);
}

/**
 * Cauchy bound: every real root satisfies |x| < 1 + max_{i<degree} |coeffs[i]/coeffs[degree]|
 * beyond it the leading term dominates, so P(x) != 0.
 */
static double root_bound(const double *coeffs, int degree)
{
    double max_term = 0.0;
    for (int i = 0; i < degree; i++) {
        double t = fabs(coeffs[i] / coeffs[degree]);
        if (t > max_term)
            max_term = t;
    }
    return 1.0 + max_term;
}

/**
 * Bisect a sign-change bracket: f_lo = P(lo) and P(hi) have opposite
 * signs (an odd-multiplicity root is inside). Each iteration
 * halves the bracket. Stops at an exact root, |P| <= f_tol, or 200
 * iterations.
 */
static double bisect(const double *coeffs, int degree, double lo, double hi,
                     double f_lo, double f_tol)
{
    LOG_V("[%g, %g]", lo, hi);
    double mid = 0.0;
    int iters;
    for (iters = 1; iters <= 200; iters++) {
        mid = 0.5 * (lo + hi);
        double f_mid = poly_eval(coeffs, degree, mid);
        if (f_mid == 0.0 || fabs(f_mid) <= f_tol)
            break;
        if ((f_mid < 0.0) == (f_lo < 0.0)) {
            lo = mid;
            f_lo = f_mid;
        } else {
            hi = mid;
        }
    }
    if (iters > 200)
        mid = 0.5 * (lo + hi);
    LOG_V("%d iter(s): x=%g, residual |P(x)|=%g", iters, mid, fabs(poly_eval(coeffs, degree, mid)));
    return mid;
}

/**
 * Newton refinement: x -= P(x)/P'(x); quadratic convergence near
 * simple roots. Stops on a zero/non-finite derivative, a non-finite
 * step, or a step <= 2 ulps of x.
 */
static double polish(const double *coeffs, int degree, double x)
{
    LOG_V("x=%g", x);
    int iters;
    for (iters = 1; iters <= 200; iters++) {
        double f = poly_eval(coeffs, degree, x);
        double d = poly_eval_deriv(coeffs, degree, x);
        if (d == 0.0 || !isfinite(f) || !isfinite(d))
            break;
        double x_new = x - f / d;
        if (!isfinite(x_new))
            break;
        if (fabs(x_new - x) <= 4.0 * DBL_EPSILON * fmax(1.0, fabs(x)))
            break;
        x = x_new;
    }
    if (x == 0.0)
        x = 0.0;
    LOG_V("%d iter(s) -> x=%g, residual |P(x)|=%g", iters, x, fabs(poly_eval(coeffs, degree, x)));
    return x;
}

/**
 * Append root to sorted roots, skipping values within ROOT_EPS of the
 * previous one (a root can surface via both the sign-change and the
 * critical-point paths).
 */
static int emit_root(double *roots, int count, double root)
{
    if (count > 0 &&
        fabs(root - roots[count - 1]) <= ROOT_EPS * fmax(1.0, fabs(root))) {
        LOG_V("duplicate of %g, skipped", roots[count - 1]);
        return count;
    }
    roots[count] = root;
    LOG_V("new root %g", root);
    return count + 1;
}

/**
 * Polish root0 and keep whichever of root0 and the polished value has
 * the smaller residual (Newton can settle on a worse point on
 * ill-conditioned flats). res0 = P(root0).
 */
static int emit_candidate(double *roots, int count, const double *coeffs,
                          int degree, double root0, double res0)
{
    double r = polish(coeffs, degree, root0);
    if (fabs(poly_eval(coeffs, degree, r)) > fabs(res0))
        r = root0;
    return emit_root(roots, count, r);
}

/**
 * Recursively find the real roots of degree-n P.
 *
 * By Rolle, P's roots are separated by those of P', so recursively
 * solving P' = 0 gives critical points that split the real line (all
 * roots lie in [-bound, bound] by Cauchy) into monotone intervals with
 * at most one root each. Per interval: a sign change -> bisect an
 * odd-multiplicity root; |P| <= f_tol at a critical point -> an even-
 * multiplicity (touching) root. Candidates are Newton-refined and
 * emitted sorted and deduplicated.
 *
 * Returns the root count (<= cap), or TOYS_SOLVE_ERR on allocation
 * failure.
 */
static int find_roots(const double *coeffs, int degree, double *roots, int cap,
                      double f_tol)
{
    LOG_V("degree %d, cap=%d, f_tol=%g", degree, cap, f_tol);
    if (degree == 0)
        return 0;

    if (degree == 1) {
        double root = -coeffs[0] / coeffs[1];
        LOG_V("linear root %g", root);
        if (cap > 0)
            roots[0] = root;
        return 1;
    }

    if (degree == 2) {
        double x1, x2;
        int count = toys_quad_solve(coeffs[2], coeffs[1], coeffs[0], &x1, &x2);
        if (count <= 0)
            return count;
        LOG_D("quadratic: %d roots", count);
        if (cap > 0)
            roots[0] = x1;
        if (count == 2 && cap > 1)
            roots[1] = x2;
        return count;
    }

    double *crit = malloc((size_t)(degree - 1) * sizeof(double));
    double *deriv = malloc((size_t)degree * sizeof(double));
    if (!crit || !deriv) {
        LOG_E("crit/deriv allocation failed (degree %d)", degree);
        free(crit);
        free(deriv);
        return TOYS_SOLVE_ERR;
    }

    for (int i = 1; i <= degree; i++)
        deriv[i - 1] = coeffs[i] * (double)i;

    LOG_V("solving P' = 0 (degree %d) for critical points", degree - 1);
    int ncrit = find_roots(deriv, degree - 1, crit, degree - 1, f_tol);

    free(deriv);
    if (ncrit < 0) {
        free(crit);
        return ncrit;
    }

    qsort(crit, (size_t)ncrit, sizeof(double), cmp_double);
    LOG_D("degree %d: %d critical points", degree, ncrit);

    for (int i = 0; i < ncrit; i++)
        LOG_V("crit[%d] = %g", i, crit[i]);

    int count = 0;
    double bound = root_bound(coeffs, degree);
    LOG_V("Cauchy bound B = %g: all real roots in [-B, B]", bound);
    double lo = -bound;
    double f_lo = poly_eval(coeffs, degree, lo);

    for (int i = 0; i < ncrit && count < cap; i++) {
        double hi = crit[i];
        double f_hi = poly_eval(coeffs, degree, hi);

        if (f_lo != 0.0 && f_hi != 0.0 && (f_lo < 0.0) != (f_hi < 0.0)) {
            double root0 = bisect(coeffs, degree, lo, hi, f_lo, f_tol);
            count = emit_candidate(roots, count, coeffs, degree, root0,
                                   poly_eval(coeffs, degree, root0));
        }

        if (count < cap && fabs(f_hi) <= f_tol)
            count = emit_candidate(roots, count, coeffs, degree, hi, f_hi);

        lo = hi;
        f_lo = f_hi;
    }

    if (count < cap) {
        double f_hi = poly_eval(coeffs, degree, bound);
        if (f_lo != 0.0 && f_hi != 0.0 && (f_lo < 0.0) != (f_hi < 0.0)) {
            double root0 = bisect(coeffs, degree, lo, bound, f_lo, f_tol);
            count = emit_candidate(roots, count, coeffs, degree, root0,
                                   poly_eval(coeffs, degree, root0));
        }
        if (count < cap && fabs(f_hi) <= f_tol)
            count = emit_candidate(roots, count, coeffs, degree, bound, f_hi);
    }

    free(crit);
    LOG_D("degree %d: found %d roots", degree, count);
    return count;
}


int toys_poly_solve(int degree, const double *coeffs, double *roots, int max_roots)
{
    assert(coeffs != NULL && roots != NULL);

    if (degree < 0 || max_roots < 0) {
        LOG_E("invalid args: degree=%d, max_roots=%d", degree, max_roots);
        return TOYS_SOLVE_ERR;
    }
    if (degree > TOYS_POLY_MAX_DEGREE) {
        LOG_E("degree %d exceeds TOYS_POLY_MAX_DEGREE %d",
              degree, TOYS_POLY_MAX_DEGREE);
        return TOYS_SOLVE_ERR;
    }

    while (degree > 0 && coeffs[degree] == 0.0)
        degree--;
    LOG_V("degree trimmed to %d", degree);
    if (degree == 0) {
        LOG_D("constant equation: %s",
              coeffs[0] == 0.0 ? "every x is a solution" : "no solutions");
        return (coeffs[0] == 0.0) ? TOYS_SOLVE_INF : 0;
    }

    double scale = 1.0;
    for (int i = 0; i <= degree; i++) {
        double abs_coeff = fabs(coeffs[i]);
        if (abs_coeff > scale)
            scale = abs_coeff;
    }
    /* residual tolerance relative to the coefficient scale, so |P(x)|
     * is judged against the magnitude of the terms being summed */
    double f_tol = 1e-9 * scale;

    int count = find_roots(coeffs, degree, roots, max_roots, f_tol);
    LOG_D("degree %d: %d roots", degree, count);
    return count;
}
