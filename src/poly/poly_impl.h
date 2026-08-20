#ifndef POLY_IMPL_H
#define POLY_IMPL_H

#include "toys/solve.h"

#include <stdbool.h>
#include <string.h>

/** Returns const polynomial. */
static inline ToysPoly toys_poly_const(double v)
{
    ToysPoly p = { 0 };
    p.coeffs[0] = v;
    return p;
}

/** Returns linear 1*x polynomial. */
static inline ToysPoly toys_poly_x(void)
{
    ToysPoly p = { 0 };
    p.degree = 1;
    p.coeffs[1] = 1.0;
    return p;
}

/** Scales all coeffs of polynomial */
static inline ToysPoly toys_poly_scale(const ToysPoly *a, double s)
{
    ToysPoly r = *a;
    for (int i = 0; i <= a->degree; i++)
        r.coeffs[i] *= s;
    return r;
}

/** Sums each coeff of 2 polynomials, result has max degree of inputs. */
static inline ToysPoly toys_poly_add(const ToysPoly *a, const ToysPoly *b)
{
    ToysPoly r = *a;
    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        r.coeffs[i] = a->coeffs[i] + b->coeffs[i];
    r.degree = degree;
    return r;
}

static inline ToysPoly toys_poly_sub(const ToysPoly *a, const ToysPoly *b)
{
    ToysPoly r = *a;
    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        r.coeffs[i] = a->coeffs[i] - b->coeffs[i];
    r.degree = degree;
    return r;
}

/* Multiplies and sums (convolutes) coefficients of polynomials,
 * returns false if resulting degree would not fit. */
static inline bool toys_poly_mul(const ToysPoly *a, const ToysPoly *b, ToysPoly *out)
{
    int degree = a->degree + b->degree;
    if (degree > TOYS_POLY_MAX_DEGREE)
        return false;

    memset(out, 0, sizeof(*out));
    out->degree = degree;
    for (int i = 0; i <= a->degree; i++)
        for (int j = 0; j <= b->degree; j++)
            out->coeffs[i + j] += a->coeffs[i] * b->coeffs[j];
    return true;
}

/* Drop trailing zero coeffs. */
static inline void toys_poly_trim(ToysPoly *p)
{
    while (p->degree > 0 && p->coeffs[p->degree] == 0.0)
        p->degree--;
}

#endif /* POLY_IMPL_H */