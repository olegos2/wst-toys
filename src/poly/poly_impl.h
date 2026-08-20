#ifndef POLY_IMPL_H
#define POLY_IMPL_H

#include "toys/solve.h"

#include <string.h>


/* Fixed-capacity polynomial shared by the solver (poly.c) and the
 * expression parser (expr.c). coeffs[i] holds the coefficient of x^i;
 * degree is the highest index the array was filled to, trailing zeroes
 * are allowed and removed by toys_poly_trim. */

static inline ToysPoly toys_poly_const(double v)
{
    ToysPoly p = { 0 };
    p.coeffs[0] = v;
    return p;
}

static inline ToysPoly toys_poly_x(void)
{
    ToysPoly p = { 0 };
    p.degree = 1;
    p.coeffs[1] = 1.0;
    return p;
}

static inline ToysPoly toys_poly_scale(const ToysPoly *a, double s)
{
    ToysPoly r = *a;
    for (int i = 0; i <= a->degree; i++)
        r.coeffs[i] *= s;
    return r;
}

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

/* Convolution; returns 0 if the result degree would exceed the solver limit. */
static inline int toys_poly_mul(const ToysPoly *a, const ToysPoly *b, ToysPoly *out)
{
    int degree = a->degree + b->degree;
    if (degree > TOYS_POLY_MAX_DEGREE)
        return 0;

    memset(out, 0, sizeof(*out));
    out->degree = degree;
    for (int i = 0; i <= a->degree; i++)
        for (int j = 0; j <= b->degree; j++)
            out->coeffs[i + j] += a->coeffs[i] * b->coeffs[j];
    return 1;
}

/** Horner evaluation of P(x) = coeffs[0] + x (coeffs[1] + ... x coeffs[degree]). */
static inline double toys_poly_eval(const ToysPoly *p, double x)
{
    double v = p->coeffs[p->degree];
    for (int i = p->degree - 1; i >= 0; i--)
        v = v * x + p->coeffs[i];
    return v;
}

/** Horner evaluation of P'(x) = sum i * coeffs[i] * x^(i-1). */
static inline double toys_poly_eval_deriv(const ToysPoly *p, double x)
{
    double v = p->degree * p->coeffs[p->degree];
    for (int i = p->degree - 1; i >= 1; i--)
        v = v * x + i * p->coeffs[i];
    return v;
}

/* Drop trailing zero coefficients; all-zero reduces to degree 0 with
 * coeffs[0] = 0, which the solver reports as TOYS_SOLVE_INF. */
static inline void toys_poly_trim(ToysPoly *p)
{
    while (p->degree > 0 && p->coeffs[p->degree] == 0.0)
        p->degree--;
}

#endif /* POLY_IMPL_H */