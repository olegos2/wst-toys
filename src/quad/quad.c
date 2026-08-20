#include "toys/quad.h"

#include <math.h>


ToysQuadResult toys_quad_solve(double a, double b, double c, double *x1, double *x2)
{
    if (a == 0.0) {
        if (b == 0.0)
            return (c == 0.0) ? TOYS_QUAD_INVALID : TOYS_QUAD_NO_ROOTS;
        *x1 = -c / b;
        *x2 = *x1;
        return TOYS_QUAD_DEGENERATE;
    }

    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return TOYS_QUAD_NO_ROOTS;

    double sqrt_disc = sqrt(disc);
    if (sqrt_disc == 0.0) {
        *x1 = -b / (2.0 * a);
        *x2 = *x1;
        return TOYS_QUAD_ONE_ROOT;
    }

    *x1 = (-b + sqrt_disc) / (2.0 * a);
    *x2 = (-b - sqrt_disc) / (2.0 * a);
    return TOYS_QUAD_TWO_ROOTS;
}
