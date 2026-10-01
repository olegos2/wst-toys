/**
 * @file
 * Type-safe stacks with integrity verification.
 *
 * Stamps the template in `stk_internal.h` once per element type. Each
 * stamp produces a struct plus `init`/`free`/`reserve`/`push`/`pop`/`verify`
 * under a per-type prefix. Provided stamps:
 * - `double`: `WstStkDouble`, `wst_stk_double_*`
 * - `int`: `WstStkInt`, `wst_stk_int_*`
 *
 * Every operation verifies length/capacity invariants, an 8-byte canary
 * tail after the buffer and a struct checksum, reporting `WstStkErr`.
 */
#ifndef TOYS_STK_H
#define TOYS_STK_H

#include <stddef.h>

/** Type to use for stack elements. */
#define STK_ELEM double

/** Name for stack struct. */
#define STK_T WstStkDouble

/** Func/var name generator in stack namespace. */
#define STK_F(name) wst_stk_double_ ## name

/** Stack element formatting. */
#define STK_FMT "%lg"

/** Generate stack for this element type. */
#include "stk_internal.h"


#define STK_ELEM int
#define STK_T WstStkInt
#define STK_F(name) wst_stk_int_ ## name
#define STK_FMT "%d"
#include "stk_internal.h"


#endif /* TOYS_STK_H */
