/**
 * @file
 * Type-safe vectors with integrity verification.
 *
 * Stamps the template in `vec_internal.h` once per element type. Each
 * stamp produces a struct plus `init`/`free`/`reserve`/`push`/`pop`/`verify`
 * and indexed `ensure`/`set`/`get`/`resize`/`clear`, plus `seal` to recompute
 * the checksum after direct buffer work, under a per-type prefix.
 * Provided stamps:
 * - `double`: `WstVecDouble`, `wst_vec_double_*`
 * - `int`: `WstVecInt`, `wst_vec_int_*`
 * - `uint8_t`: `WstVecU8`, `wst_vec_u8_*`
 * - `uint32_t`: `WstVecU32`, `wst_vec_u32_*``
 *
 * Every operation verifies length/capacity invariants, canary blocks on
 * both sides of the buffer and a struct checksum, reporting `WstVecErr`.
 */
#ifndef TOYS_VEC_H
#define TOYS_VEC_H

#include <stdint.h>


/** Type to use for vector elements. */
#define VEC_ELEM double

/** Name for vector struct. */
#define VEC_T WstVecDouble

/** Func/var name generator in vector namespace. */
#define VEC_F(name) wst_vec_double_ ## name

/** Vector element formatting. */
#define VEC_FMT "%lg"

/** Generate vector for this element type. */
#include "vec_internal.h"


#define VEC_ELEM int
#define VEC_T WstVecInt
#define VEC_F(name) wst_vec_int_ ## name
#define VEC_FMT "%d"
#include "vec_internal.h"


#define VEC_ELEM uint8_t
#define VEC_T WstVecU8
#define VEC_F(name) wst_vec_u8_ ## name
#define VEC_FMT "%hhu"
#include "vec_internal.h"


#define VEC_ELEM uint32_t
#define VEC_T WstVecU32
#define VEC_F(name) wst_vec_u32_ ## name
#define VEC_FMT "%u"
#include "vec_internal.h"


#endif /* TOYS_VEC_H */
