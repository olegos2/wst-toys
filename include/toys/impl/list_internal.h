/**
 * Pointer/block list backend.
 *
 * Live nodes are linked through `next`/`prev`, and free nodes form a singly
 * linked pool. Nodes live in geometrically growing, never-resized blocks, so
 * public node pointers stay valid across insertions and only become stale
 * after `list_clear` or `list_free`. The first insertion allocates the first
 * block.
 *
 * See `list_idx_internal.h` for the flat-array backend and
 * `list_common_internal.h` for the stamp macro contract.
 */

#include "list_common_internal.h"


/*
 * Only the constants are guarded. The stamp below expands again for each
 * LIST_T include.
 */
#ifndef TOYS_LIST_INTERNAL_H
#define TOYS_LIST_INTERNAL_H

/** Capacity of the first block of nodes. */
#define WST_LIST_BLOCK_INIT_CAP 8

#endif /* TOYS_LIST_INTERNAL_H */

/*
 * Private declarations for list implementation.
 */
#ifdef LIST_IMPL

#include "vec_internal.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Translate a block-vector error into a list error. */
WstListErr wst_list_err_from_vec(WstVecErr err);

#endif /* LIST_IMPL */

#ifdef LIST_T

/* Backend-specific helpers, kept in the generated namespace. */
#define list_drop_blocks LIST_F(drop_blocks)
#define list_new_block LIST_F(new_block)
#define list_alloc_node LIST_F(alloc_node)

#define LIST_BLOCK_T WST_LIST_CAT(LIST_T, Block)
#define LIST_BLOCK_PTR_T WST_LIST_CAT(LIST_T, BlockPtr)

/** A list node holding one element of the stamped type. */
typedef struct {
    WstListNodeHdr hdr;
    LIST_ELEM data;
} LIST_NODE_T;

/** A heap block of node slots, blocks are never resized. */
typedef struct {
    /** Number of node slots in this block. */
    size_t cap;
    _Alignas(LIST_NODE_T) LIST_NODE_T nodes[];
} LIST_BLOCK_T;

typedef LIST_BLOCK_T *LIST_BLOCK_PTR_T;

/* Node blocks are tracked in a vector of pointers. */
#define IMPL2_VEC_T(a, b) a ## _ ## b
#define IMPL_VEC_T(a, b) IMPL2_VEC_T(a, b)
#define BLOCK_VEC_T IMPL_VEC_T(WstVecListBlock, LIST_ELEM)

#define IMPL2_BLOCK_VEC_F(elem, name) wst_vec_list_block_ ## elem ## _ ## name
#define IMPL_BLOCK_VEC_F(elem, name) IMPL2_BLOCK_VEC_F(elem, name)
#define BLOCK_VEC_F(name) IMPL_BLOCK_VEC_F(LIST_ELEM, name)

/*
 * Stamp a vector of block pointers. LIST_IMPL and VEC_IMPL are forwarded so
 * both templates emit definitions when needed.
 */
#define VEC_ELEM LIST_BLOCK_PTR_T
#define VEC_T BLOCK_VEC_T
#define VEC_F(name) BLOCK_VEC_F(name)
#define VEC_REF

#if defined(LIST_IMPL) && !defined(VEC_IMPL)
#  define VEC_IMPL
#  include "vec_internal.h"
#  undef VEC_IMPL
#elif !defined(LIST_IMPL) && defined(VEC_IMPL)
#  undef VEC_IMPL
#  include "vec_internal.h"
#  define VEC_IMPL
#else
#  include "vec_internal.h"
#endif /* LIST_IMPL ^ VEC_IMPL */

/** Doubly linked list. */
typedef struct {
    WstListHdr hdr;
    BLOCK_VEC_T blocks;
} LIST_T;

/*
 * Raw primitives consumed by the shared typed API. They precede the API
 * include because its common wrappers call them.
 */
#ifdef LIST_IMPL

/** Get a node without verifying the list. */
static LIST_NODE_T *list_node_at_raw(const LIST_T *list, size_t idx)
{
    assert(list != NULL);
    return (LIST_NODE_T *)wst_list_hdr_at(&list->hdr, idx);
}

/** Find the first node holding `val`, without verifying the list. */
static LIST_NODE_T *list_find_node_raw(const LIST_T *list,
                                       const LIST_ELEM *val)
{
    return (LIST_NODE_T *)wst_list_hdr_find(&list->hdr,
                                            offsetof(LIST_NODE_T, data), val,
                                            sizeof(LIST_ELEM));
}

/** Find the first matching index, without verifying the list. */
static ssize_t list_find_index_raw(const LIST_T *list, const LIST_ELEM *val)
{
    return wst_list_hdr_find_index(&list->hdr, offsetof(LIST_NODE_T, data),
                                   val, sizeof(LIST_ELEM));
}

static WstListErr list_drop_blocks(LIST_T *list)
{
    assert(list != NULL);

    size_t count = list->blocks.length;
    LOG_D("dropping %zu node blocks", count);

    for (size_t i = 0; i < count; i++) {
        LIST_BLOCK_T *block = list->blocks.data[i];
        LOG_V("freeing block %zu/%zu at %p (%zu nodes)", i + 1, count,
              (const void *)block, block->cap);
        free(block);
    }

    WstVecErr verr = BLOCK_VEC_F(free)(&list->blocks);
    if (verr != WST_VEC_NO_ERR) {
        LOG_E("cannot free block vector: %s", wst_vec_err_str(verr));
        return wst_list_err_from_vec(verr);
    }

    LOG_D("dropped node blocks and block vector");
    return WST_LIST_NO_ERR;
}

static WstListErr list_new_block(LIST_T *list)
{
    assert(list != NULL);

    size_t cap = WST_LIST_BLOCK_INIT_CAP;
    if (list->blocks.length > 0) {
        size_t prev_cap = list->blocks.data[list->blocks.length - 1]->cap;
        if (prev_cap > SIZE_MAX / 2) {
            LOG_E("block capacity %zu overflows size_t", prev_cap);
            return WST_LIST_ERR_OVERFLOW;
        }
        cap = prev_cap * 2;
    }

    if (cap == 0)
        cap = 1;

    if (cap > (SIZE_MAX - sizeof(LIST_BLOCK_T)) / sizeof(LIST_NODE_T)) {
        LOG_E("block capacity %zu overflows size_t", cap);
        return WST_LIST_ERR_OVERFLOW;
    }

    LIST_BLOCK_T *block = malloc(sizeof(LIST_BLOCK_T) +
                                 cap * sizeof(LIST_NODE_T));
    if (block == NULL) {
        LOG_E("cannot allocate %zu-node block: %s", cap, strerror(errno));
        return WST_LIST_ERR_NOMEM;
    }
    block->cap = cap;

    LIST_BLOCK_PTR_T block_ptr = block;
    WstVecErr verr = BLOCK_VEC_F(push)(&list->blocks, &block_ptr);
    if (verr != WST_VEC_NO_ERR) {
        free(block);
        return wst_list_err_from_vec(verr);
    }

    wst_list_hdr_add_free(&list->hdr, block->nodes, block->cap,
                          sizeof(LIST_NODE_T));
    LOG_D("allocated block %zu with %zu nodes at %p",
          list->blocks.length, block->cap, (const void *)block);
    return WST_LIST_NO_ERR;
}

static WstListErr list_alloc_node(LIST_T *list, LIST_NODE_T **out)
{
    assert(list != NULL);
    assert(out != NULL);

    WstListNodeHdr *hdr = wst_list_hdr_take_free(&list->hdr);
    if (hdr == NULL) {
        WstListErr err = list_new_block(list);
        if (err != WST_LIST_NO_ERR)
            return err;

        hdr = wst_list_hdr_take_free(&list->hdr);
        assert(hdr != NULL);
    }

    *out = (LIST_NODE_T *)hdr;
    return WST_LIST_NO_ERR;
}

static WstListErr list_insert_after_node_raw(LIST_T *list,
                                             const LIST_NODE_T *prev,
                                             const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    LIST_NODE_T *new_node = NULL;
    WstListErr err = list_alloc_node(list, &new_node);
    if (err != WST_LIST_NO_ERR)
        return err;

    new_node->data = *val;
    wst_list_hdr_insert_after(&list->hdr, (WstListNodeHdr *)prev,
                              (WstListNodeHdr *)new_node);
    return WST_LIST_NO_ERR;
}

/** Append a copy of `val` after the current tail. */
static WstListErr list_append_raw(LIST_T *list, const LIST_ELEM *val)
{
    return list_insert_after_node_raw(
        list, (const LIST_NODE_T *)list->hdr.tail, val);
}

#endif /* LIST_IMPL */

#include "list_api_internal.h"

/* Storage-specific public API. */
#ifdef LIST_IMPL

WstListErr list_free(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    LOG_D("freeing list with %zu node blocks", list->blocks.length);

    err = list_drop_blocks(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    wst_list_hdr_reset(&list->hdr);
    LOG_D("freed list");
    return WST_LIST_NO_ERR;
}

WstListErr list_clear(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    wst_list_hdr_reset(&list->hdr);

    for (size_t i = 0; i < list->blocks.length; i++) {
        LIST_BLOCK_T *block = list->blocks.data[i];
        LOG_V("pooling block %zu/%zu at %p (%zu nodes)", i + 1,
              list->blocks.length, (const void *)block, block->cap);
        wst_list_hdr_add_free(&list->hdr, block->nodes, block->cap,
                              sizeof(LIST_NODE_T));
    }

    LOG_D("cleared list, %zu blocks pooled", list->blocks.length);
    return WST_LIST_NO_ERR;
}

WstListErr list_init(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    err = list_drop_blocks(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    wst_list_hdr_reset(&list->hdr);

    WstVecErr verr = BLOCK_VEC_F(reserve)(&list->blocks, 1);
    if (verr != WST_VEC_NO_ERR)
        return wst_list_err_from_vec(verr);

    LOG_D("initialized empty list with block vector capacity %zu",
          list->blocks.cap);
    return WST_LIST_NO_ERR;
}

WstListErr list_verify(const LIST_T *list)
{
    if (list == NULL) {
        LOG_E("cannot verify null list");
        return WST_LIST_ERR_NULL;
    }

    return wst_list_err_from_vec(BLOCK_VEC_F(verify)(&list->blocks));
}

void list_dump(const LIST_T *list)
{
    LOG_V("Dumping list at %p", (const void *)list);

    if (list == NULL) {
        fprintf(stderr, "list: <null>\n");
        return;
    }

    fprintf(stderr, "list <%p>:\n"
            "  head: %p\n"
            "  tail: %p\n"
            "  free: %p\n",
            (const void *)list, (const void *)list->hdr.head,
            (const void *)list->hdr.tail, (const void *)list->hdr.free);

    bool sane = list_verify(list) == WST_LIST_NO_ERR;

#ifdef LIST_FMT
    if (sane) {
        fprintf(stderr, "  elements:\n");
        size_t i = 0;
        for (const WstListNodeHdr *hdr = list->hdr.head; hdr != NULL;
             hdr = hdr->next, i++)
            fprintf(stderr, "    [%4zu]: " LIST_FMT "\n", i,
                    ((const LIST_NODE_T *)hdr)->data);
    } else {
        fprintf(stderr, "  elements: omitted due to verification failure\n");
    }
#endif

    if (sane) {
        fprintf(stderr, "  blocks (%zu):\n", list->blocks.length);
        for (size_t i = 0; i < list->blocks.length; i++)
            fprintf(stderr, "    [%4zu]: %zu nodes\n", i,
                    list->blocks.data[i]->cap);
    } else {
        fprintf(stderr, "  blocks: omitted due to verification failure\n");
    }

    BLOCK_VEC_F(dump)(&list->blocks);
}

bool list_remove_node(LIST_T *list, const LIST_NODE_T *node)
{
    assert(list != NULL);

    if (node == NULL)
        return false;

    if (list_verify(list) != WST_LIST_NO_ERR)
        return false;

    wst_list_hdr_remove(&list->hdr, (WstListNodeHdr *)node);
    LOG_V("removed node %p", (const void *)node);
    return true;
}

WstListErr list_pop(LIST_T *list, LIST_ELEM *out)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    if (list->hdr.tail == NULL) {
        LOG_E("pop from an empty list");
        return WST_LIST_ERR_EMPTY;
    }

    LIST_NODE_T *node = (LIST_NODE_T *)list->hdr.tail;
    if (out != NULL)
        *out = node->data;

    wst_list_hdr_remove(&list->hdr, list->hdr.tail);
    LOG_V("pop, %zu blocks total", list->blocks.length);
    return WST_LIST_NO_ERR;
}

#endif /* LIST_IMPL */

#undef list_free
#undef list_clear
#undef list_init
#undef list_verify
#undef list_dump
#undef list_get_node
#undef list_get
#undef list_remove_node
#undef list_remove
#undef list_pop
#undef list_find_node
#undef list_find
#undef list_set
#undef list_append
#undef list_insert_by_node
#undef list_insert

#undef list_drop_blocks
#undef list_new_block
#undef list_alloc_node
#undef list_node_at_raw
#undef list_find_node_raw
#undef list_find_index_raw
#undef list_append_raw
#undef list_insert_after_node_raw

#undef LIST_NODE_T
#undef LIST_BLOCK_T
#undef LIST_BLOCK_PTR_T

#undef IMPL2_VEC_T
#undef IMPL_VEC_T
#undef BLOCK_VEC_T

#undef IMPL2_BLOCK_VEC_F
#undef IMPL_BLOCK_VEC_F
#undef BLOCK_VEC_F

#undef LIST_REF
#undef LIST_FMT
#undef LIST_F
#undef LIST_ELEM
#undef LIST_T

#endif /* LIST_T */
