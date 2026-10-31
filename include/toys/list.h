/**
 * @file
 * Public stamp for the pointer/block list backend.
 *
 * Provides `WstListInt` and `wst_list_int_*`. Every operation reports
 * `WstListErr`. See `impl/list_internal.h` for storage and lifetime details.
 */
#ifndef TOYS_LIST_H
#define TOYS_LIST_H

/** Type to use for list elements. */
#define LIST_ELEM int

/** Name for list struct. */
#define LIST_T WstListInt

/** Func/var name generator in list namespace. */
#define LIST_F(name) wst_list_int_ ## name

/** List element formatting. */
#define LIST_FMT "%d"

/** Generate list for this element type. */
#include "impl/list_internal.h"

#endif /* TOYS_LIST_H */
