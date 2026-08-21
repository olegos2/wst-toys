#include "toys/debug.h"
#include "toys/solve.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

/* `iszero` uses are questionable? */

void toys_poly_trim(ToysPoly *p)
{
    // /* Clamp bad degree number. */
    // if (p->degree > TOYS_POLY_MAX_DEGREE)
    //     p->degree = TOYS_POLY_MAX_DEGREE;
    while (p->degree > 0 && iszero(p->coeffs[p->degree]))
        p->degree--;
}

static int toys_quad_solve(double c, double b, double a, double *x1, double *x2)
{
    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return 0;

    double sqrt_disc = sqrt(disc);
    if (iszero(sqrt_disc)) {
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

ToysSolution toys_poly_solve(ToysPoly *poly)
{
    assert(poly != NULL);
    ToysSolution sol = { 0 };

    toys_poly_trim(poly);
    switch (poly->degree) {
    case 0:
        sol.count = iszero(poly->coeffs[0]) ? TOYS_SOLVE_INF : 0;
        return sol;
    case 1:
        sol.roots[0] = -poly->coeffs[0] / poly->coeffs[1];
        sol.count = 1;
        return sol;
    case 2:
        sol.count = toys_quad_solve(
            poly->coeffs[0], poly->coeffs[1], poly->coeffs[2], &sol.roots[0], &sol.roots[1]);
        return sol;
    default:
        LOG_E("invalid args: degree=%d", poly->degree);
        sol.count = TOYS_SOLVE_ERR;
        return sol;
    }
}
