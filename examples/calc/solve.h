#ifndef CALC_SOLVE_H
#define CALC_SOLVE_H

#include "toys/poly.h"

#include <stddef.h>

/** Capacity of string that holds pretty polynomial expr. */
#define POLY_BUF_LEN 256

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

int solve_run_plot(size_t npolys, const WstPoly *polys, const WstSolution *sols);

#endif /* CALC_SOLVE_H */
