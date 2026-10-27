/**
 * @file
 * Type-safe doubly linked lists backed by geometric node blocks.
 *
 * Live nodes are linked through `next`/`prev`; removed nodes form a singly
 * linked free pool inside the node blocks. Blocks are allocated on demand and
 * never resized, so node addresses stay stable across insertions. Public
 * operations cover lifecycle, node-handle access and mutation, index/value
 * access, and tail `append`/`pop`.
 *
 * Provided stamps:
 * - `int`: `WstListInt`, `wst_list_int_*`
 *
 * Every operation verifies the block-pointer vector, reporting `WstListErr`.
 * Node pointers are invalidated by `list_free` and become stale after
 * `list_clear`.
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
#include "list_internal.h"

#endif /* TOYS_LIST_H */