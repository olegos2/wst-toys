#include "toys/debug.h"
#include "toys/solve.h"
#include "toys/math.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

/* `iszero` uses are questionable? */
/* poly degree is int and not limited to never be < 0, so unexpected things may happen */
/* wst_poly_solve is the only one that trims poly, have to keep this in mind. */

void wst_poly_trim(WstPoly *p)
{
    assert(p != NULL);
    // /* Clamp bad degree number. */
    // if (p->degree > WST_SOLVE_MAX_DEGREE)
    //     p->degree = WST_SOLVE_MAX_DEGREE;
    while (p->degree > 0 && my_iszero(p->coeffs[p->degree]))
        p->degree--;
}

void wst_poly_scale(WstPoly *a, double s)
{
    assert(a != NULL);

    for (int i = 0; i <= a->degree; i++)
        a->coeffs[i] *= s;
    wst_poly_trim(a);
}

void wst_poly_add(WstPoly *a, const WstPoly *b)
{
    assert(a != NULL);
    assert(b != NULL);

    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        a->coeffs[i] += b->coeffs[i];
    a->degree = degree;
    wst_poly_trim(a);
}

void wst_poly_sub(WstPoly *a, const WstPoly *b)
{
    assert(a != NULL);
    assert(b != NULL);

    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        a->coeffs[i] -= b->coeffs[i];
    a->degree = degree;
    wst_poly_trim(a);
}

bool wst_poly_mul(WstPoly *a, const WstPoly *b)
{
    assert(a != NULL);
    assert(b != NULL);

    WstPoly out = { 0 };

    int degree = a->degree + b->degree;
    if (degree > WST_SOLVE_MAX_DEGREE)
        return false;

    out.degree = degree;
    for (int i = 0; i <= a->degree; i++)
        for (int j = 0; j <= b->degree; j++)
            out.coeffs[i + j] += a->coeffs[i] * b->coeffs[j];
    *a = out;
    return true;
}

WstPoly wst_poly_deriv(const WstPoly *poly)
{
    assert(poly != NULL);
    WstPoly out = { 0 };

    if (poly->degree <= 0)
        return out;

    out.degree = poly->degree - 1;
    for (int i = 0; i <= out.degree; i++)
        out.coeffs[i] = poly->coeffs[i + 1] * (double)(i + 1);
    return out;
}

bool wst_poly_integ(const WstPoly *poly, WstPoly *out)
{
    assert(poly != NULL);
    assert(out != NULL);

    if (poly->degree < 0 || poly->degree > WST_SOLVE_MAX_DEGREE - 1) {
        LOG_E("Cannot integrate poly degree %d", poly->degree);
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->coeffs[0] = 0.0;
    out->degree = poly->degree + 1;
    for (int i = 0; i <= poly->degree; i++)
        out->coeffs[i + 1] = poly->coeffs[i] / (double)(i + 1);

    return true;
}

double wst_poly_eval(const WstPoly *poly, double x)
{
    assert(poly != NULL);

    double ret = 0;

    double s = 1;
    for (int i = 0; i <= poly->degree; i++) {
        ret += poly->coeffs[i] * s;
        s *= x;
    }

    return ret;
}

/**
 * @return number of roots
 */
static int wst_quad_solve(double c, double b, double a, double *x1, double *x2)
{
    assert(x1 != NULL);
    assert(x2 != NULL);
    assert(x1 != x2);

    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return 0;

    double sqrt_disc = sqrt(disc);
    if (my_iszero(sqrt_disc)) {
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

WstSolution wst_solve_poly(WstPoly *poly)
{
    assert(poly != NULL);
    WstSolution sol = { 0 };

    wst_poly_trim(poly);
    switch (poly->degree) {
    case 0:
        sol.count = my_iszero(poly->coeffs[0]) ? WST_SOLVE_INF : 0;
        return sol;
    case 1:
        sol.roots[0] = -poly->coeffs[0] / poly->coeffs[1];
        sol.count = 1;
        return sol;
    case 2:
        sol.count = wst_quad_solve(
            poly->coeffs[0], poly->coeffs[1], poly->coeffs[2], &sol.roots[0], &sol.roots[1]);
        return sol;
    default:
        LOG_E("invalid args: degree=%d", poly->degree);
        sol.count = WST_SOLVE_ERR;
        return sol;
    }
}
