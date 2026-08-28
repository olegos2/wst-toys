#include "toys/debug.h"
#include "toys/poly.h"
#include "toys/math.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* `iszero` uses are questionable? */
/* poly degree is int and not limited to never be < 0, so unexpected things may happen. */
/* Whenever polynomial degree may increase, output degree must be checked (trimmed). */

void wst_poly_trim(WstPoly *p)
{
    assert(p != NULL);

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
    if (degree > WST_POLY_MAX_DEGREE)
        return false;

    out.degree = degree;
    for (int i = 0; i <= a->degree; i++)
        for (int j = 0; j <= b->degree; j++)
            out.coeffs[i + j] += a->coeffs[i] * b->coeffs[j];
    *a = out;
    return true;
}

bool wst_poly_cmp(const WstPoly *a, const WstPoly *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->degree != b->degree) return false;

    for (int i = 0; i <= a->degree; i++)
        if (!my_iszero(a->coeffs[i] - b->coeffs[i]))
            return false;

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

    if (poly->degree < 0 || poly->degree > WST_POLY_MAX_DEGREE - 1) {
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

WstSolution wst_poly_solve(WstPoly *poly)
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

typedef struct {
    char *cur;
    size_t rem;
} StringPosition;

static void str_append(StringPosition *pos, const char *fmt, ...)
{
    va_list args = { 0 };
    va_start(args, fmt);
    int written = vsnprintf(pos->cur, pos->rem, fmt, args);
    va_end(args);

    if (written > 0) {
        size_t u_written = (size_t)written;
        if (u_written >= pos->rem)
            u_written = pos->rem;
        pos->cur += u_written;
        pos->rem -= u_written;
    }
}

void wst_poly_print(char *buf, size_t nbuf, const char *name,
                    const WstPoly *poly, bool pretty)
{
    StringPosition p = { .cur = buf, .rem = nbuf };
    
    if (!pretty) {
        for (int i = 0; i <= poly->degree; i++)
            str_append(&p, "%lg ", poly->coeffs[i]);
        return;
    }

    str_append(&p, "%s(x) = ", name);

    bool first = true;
    for (int i = poly->degree; i >= 0; i--) {
        if (my_iszero(poly->coeffs[i])) continue;

        if (!first)
            str_append(&p, poly->coeffs[i] < 0.0 ? " - " : " + ");
        else if (poly->coeffs[i] < 0.0)
            str_append(&p, "-");

        double a = fabs(poly->coeffs[i]);
        if (i == 0 || !my_iszero(a - 1.0))
            str_append(&p, "%lg*", a);
        if (i > 0)
            str_append(&p, "x");
        if (i > 1)
            str_append(&p, "^%d", i);
        first = false;
    }
    if (first)
        str_append(&p, "0");

#undef ADD
}
