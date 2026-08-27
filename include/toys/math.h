#ifndef TOYS_MATH_H
#define TOYS_MATH_H

#include <assert.h>
#include <stdbool.h>
#include <float.h>
#include <stdint.h>


/** Check if `double` is in `-DBL_EPSILON..DBL_EPSILON` range. */
static inline bool my_iszero(double a)
{
    return a > -DBL_EPSILON && a < DBL_EPSILON;
}

/** Representation of IEEE double with sign, exponent and significand */
typedef union {
    double val;
    struct {
        uint64_t sign : 1;
        uint64_t exp : 11;
        uint64_t significand : 52;
    };
} double_repr;

/** Replacements for builtin macros for no reason (for learning or smth) */
static inline bool my_isnan(double a)
{
    assert(sizeof(double) == 8);
    /* Can be done without bitfields, but why not */
    double_repr repr = (double_repr)a;
    return repr.sign == 0 && repr.exp == 0x7FF && (
        repr.significand == 1 || repr.significand == ((1ull << 51) | 1) || repr.significand == ((1ull << 52) - 1));
}

static inline bool my_isinf(double a)
{
    assert(sizeof(double) == 8);
    double_repr repr = (double_repr)a;
    return repr.exp == 0x7FF && repr.significand == 0;
}

#endif /* TOYS_MATH_H */
