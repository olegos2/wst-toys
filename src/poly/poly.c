#include "toys/debug.h"
#include "toys/solve.h"
#include "poly_impl.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


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

/** qsort comparator for doubles; NaN compares equal to everything. */
static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa;
    double b = *(const double *)pb;
    return (a > b) - (a < b);
}

/**
 * Cauchy bound: every real root satisfies |x| < 1 + max_{i<degree}
 * |coeffs[i] / coeffs[degree]|, i.e. beyond it the leading term
 * dominates and P(x) != 0.
 *
 * @param [in] p polynomial
 */
static double root_bound(const ToysPoly *p)
{
    double max_term = 0.0;
    for (int i = 0; i < p->degree; i++) {
        double t = fabs(p->coeffs[i] / p->coeffs[p->degree]);
        if (t > max_term)
            max_term = t;
    }
    return 1.0 + max_term;
}

/**
 * Bisect a sign-change bracket: f_lo = P(lo) and P(hi) have opposite
 * signs, so by the intermediate value theorem an odd-multiplicity root
 * is inside. Each iteration halves the bracket; stops at an exact
 * root, |P| <= f_tol, or 200 iterations.
 *
 * @param [in] p polynomial
 * @param [in] lo bracket start
 * @param [in] hi bracket end
 * @param [in] f_lo P(lo)
 * @param [in] f_tol residual tolerance
 */
static double bisect(const ToysPoly *p, double lo, double hi,
                     double f_lo, double f_tol)
{
    LOG_V("[%g, %g]", lo, hi);

    double mid = 0.0;
    int iters;
    for (iters = 1; iters <= 200; iters++) {
        mid = 0.5 * (lo + hi);
        double f_mid = toys_poly_eval(p, mid);
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

    LOG_V("%d iter(s): x=%g, residual |P(x)|=%g", iters, mid,
          fabs(toys_poly_eval(p, mid)));
    return mid;
}

/**
 * Newton refinement x -= P(x)/P'(x); converges quadratically near
 * simple roots. Stops on a zero or non-finite derivative, a non-finite
 * step, or a step within 2 ulps of x (already at full precision).
 *
 * @param [in] p polynomial
 * @param [in] x starting point
 */
static double polish(const ToysPoly *p, double x)
{
    LOG_V("x=%g", x);

    int iters;
    for (iters = 1; iters <= 200; iters++) {
        double f = toys_poly_eval(p, x);
        double d = toys_poly_eval_deriv(p, x);
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

    LOG_V("%d iter(s) -> x=%g, residual |P(x)|=%g", iters, x,
          fabs(toys_poly_eval(p, x)));
    return x;
}

/**
 * Append root to the sorted roots array, skipping values within
 * ROOT_EPS of the previous entry (a root can surface via both the
 * sign-change and the critical-point paths).
 *
 * @param [in] roots output array, sorted ascending
 * @param [in] count current number of roots in roots
 * @param [in] root candidate to append
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
 * the smaller residual, so Newton can't settle on a worse point on
 * ill-conditioned flats (res0 = P(root0)).
 *
 * @param [in] roots output array, sorted ascending
 * @param [in] count current number of roots in roots
 * @param [in] p polynomial
 * @param [in] root0 root candidate from bisection or a critical point
 * @param [in] res0 P(root0)
 */
static int emit_candidate(double *roots, int count, const ToysPoly *p,
                          double root0, double res0)
{
    double r = polish(p, root0);
    if (fabs(toys_poly_eval(p, r)) > fabs(res0))
        r = root0;
    return emit_root(roots, count, r);
}

/**
 * Recursively find the real roots of degree-n P.
 *
 * By Rolle's theorem, every pair of distinct roots of P has a root of
 * P' between them, so the roots of P' split the real line (all roots
 * lie within the Cauchy bound) into monotone intervals with at most
 * one root of P each. Per interval: a sign change implies an
 * odd-multiplicity root, found by bisection; |P| <= f_tol at a
 * critical point implies an even-multiplicity (touching) root.
 * Candidates are Newton-refined, then emitted sorted and deduplicated.
 *
 * @param [in] p polynomial
 * @param [out] roots founded roots in ascending order
 * @param [in] cap capacity of roots
 * @param [in] f_tol residual tolerance
 *
 * @return number of roots (<= cap), or TOYS_SOLVE_ERR on allocation
 *         failure
 */
static int find_roots(const ToysPoly *p, double *roots, int cap, double f_tol)
{
    LOG_V("degree %d, cap=%d, f_tol=%g", p->degree, cap, f_tol);

    if (p->degree == 0)
        return 0;

    if (p->degree == 1) {
        double root = -p->coeffs[0] / p->coeffs[1];
        LOG_V("linear root %g", root);
        if (cap > 0)
            roots[0] = root;
        return 1;
    }

    if (p->degree == 2) {
        double x1, x2;
        int count = toys_quad_solve(p->coeffs[2], p->coeffs[1], p->coeffs[0],
                                    &x1, &x2);
        if (count <= 0)
            return count;
        LOG_D("quadratic: %d roots", count);
        if (cap > 0)
            roots[0] = x1;
        if (count == 2 && cap > 1)
            roots[1] = x2;
        return count;
    }

    int degree = p->degree;
    double *crit = malloc((size_t)(degree - 1) * sizeof(double));
    double *deriv = malloc((size_t)degree * sizeof(double));
    if (!crit || !deriv) {
        LOG_E("crit/deriv allocation failed (degree %d)", degree);
        free(crit);
        free(deriv);
        return TOYS_SOLVE_ERR;
    }

    for (int i = 1; i <= degree; i++)
        deriv[i - 1] = p->coeffs[i] * (double)i;

    ToysPoly derivative = { 0 };
    memcpy(derivative.coeffs, deriv, (size_t)degree * sizeof(double));
    derivative.degree = degree - 1;

    LOG_V("solving P' = 0 (degree %d) for critical points", derivative.degree);
    int ncrit = find_roots(&derivative, crit, degree - 1, f_tol);

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
    double bound = root_bound(p);
    LOG_V("Cauchy bound B = %g: all real roots in [-B, B]", bound);

    double lo = -bound;
    double f_lo = toys_poly_eval(p, lo);
    for (int i = 0; i < ncrit && count < cap; i++) {
        double hi = crit[i];
        double f_hi = toys_poly_eval(p, hi);

        if (f_lo != 0.0 && f_hi != 0.0 && (f_lo < 0.0) != (f_hi < 0.0)) {
            double root0 = bisect(p, lo, hi, f_lo, f_tol);
            count = emit_candidate(roots, count, p, root0,
                                   toys_poly_eval(p, root0));
        }

        if (count < cap && fabs(f_hi) <= f_tol)
            count = emit_candidate(roots, count, p, hi, f_hi);

        lo = hi;
        f_lo = f_hi;
    }

    if (count < cap) {
        double f_hi = toys_poly_eval(p, bound);
        if (f_lo != 0.0 && f_hi != 0.0 && (f_lo < 0.0) != (f_hi < 0.0)) {
            double root0 = bisect(p, lo, bound, f_lo, f_tol);
            count = emit_candidate(roots, count, p, root0,
                                   toys_poly_eval(p, root0));
        }
        if (count < cap && fabs(f_hi) <= f_tol)
            count = emit_candidate(roots, count, p, bound, f_hi);
    }

    free(crit);
    LOG_D("degree %d: found %d roots", degree, count);
    return count;
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

    if (p.degree == 0) {
        LOG_D("constant equation: %s",
              p.coeffs[0] == 0.0 ? "every x is a solution" : "no solutions");
        return (p.coeffs[0] == 0.0) ? TOYS_SOLVE_INF : 0;
    }

    /* residual tolerance relative to the coefficient scale */
    double scale = 1.0;
    for (int i = 0; i <= p.degree; i++) {
        double abs_coeff = fabs(p.coeffs[i]);
        if (abs_coeff > scale)
            scale = abs_coeff;
    }
    double f_tol = 1e-9 * scale;

    int count = find_roots(&p, roots, max_roots, f_tol);
    LOG_D("degree %d: %d roots", p.degree, count);
    return count;
}