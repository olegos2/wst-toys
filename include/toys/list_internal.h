#ifndef TOYS_LIST_INTERNAL_H
#define TOYS_LIST_INTERNAL_H

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


/**
 * A header that generates typed doubly linked lists,
 * Uses macros to describe list element type.
 * These macros must be defined:
 * `LIST_ELEM`    - type to use for list elements
 * `LIST_T`       - name for list struct.
 * `LIST_F(name)` - func/var name generator in list namespace.
 * `LIST_FMT`     - list element format specifier/string, unless `LIST_REF`.
 * `LIST_REF`     - optional, pass elements by pointer instead of by value.
 *
 * Use `LIST_IMPL` macro in one TU where you want a list type
 * to be implemented.
 *
 * Lists are implemented as struct with backing store for nodes and
 * a header. Header contains pointers to head, tail and free nodes.
 *
 * All allocated, but not live (free) nodes are chained in their own
 * linked list.
 *
 * Backing store is organized as vector of chunks (blocks) of nodes.
 * Each chunk has its own capacity and is never reallocated.
 * Their individual capacities grow geometrically with every new chunk.
 *
 * Initial state of the list is NULL-ed header and 0 blocks.
 * First element added will add a new block of initial capacity, then take
 * first free node from that block, copy passed value to that node, then
 * add it to the back of the list.
 *
 * Whenever there's no free nodes left and a new element needs to be added,
 * a new block is reserved. Capacity of that block is double the capacity of
 * previously allocated block.
 * After allocating the new block, all nodes in it are linked in one direction,
 * then front of this chain becomes the next free node.
 *
 * Whenever a new element needs to be added in the middle, a node is popped from
 * the front of free nodes chain, and linked between two live nodes of the list.
 *
 * First node always has `prev` set to `NULL`, last node has `next` set to `NULL`.
 *
 * Clearing the list means linking nodes throughout all blocks into free nodes chain,
 * setting tail and head to `NULL`, and `hdr.free` points to the first free node.
 *
 * Freeing the list means freeing all blocks, freeing block pointers vector and
 * NULL-ing the list header.
 */


// /** Canary tail size in bytes, shared by all instantiations. */
// #define LIST_CANARY_SIZE 8

/** Node capacity of the first block; later blocks double. */
#define LIST_BLOCK_INIT_CAP 8

// #if !defined(LIST_USE_CANARY)
// #  define LIST_USE_CANARY 0
// #endif

// #if !defined(LIST_USE_HASH)
// #  define LIST_USE_HASH 0
// #endif

// #if !defined(LIST_USE_CONTENT_HASH)
// #  define LIST_USE_CONTENT_HASH 0
// #endif

// #if LIST_USE_CANARY
// #  define LIST_HEAD_SIZE LIST_CANARY_SIZE
// #  define LIST_TAIL_SIZE LIST_CANARY_SIZE
// #else
// #  define LIST_HEAD_SIZE 0
// #  define LIST_TAIL_SIZE 0
// #endif

// #define LIST_CANARY_TOTAL (LIST_HEAD_SIZE + LIST_TAIL_SIZE)


/**
 * Error that may occur while working with a list.
 * Should be always handled after every operation.
 */
typedef enum {
    WST_LIST_NO_ERR = 0,
    /** Allocation failure; all operations but init keep previous contents. */
    WST_LIST_ERR_NOMEM,
    /** Pop from an empty list. */
    WST_LIST_ERR_EMPTY,
    /** Null list passed. */
    WST_LIST_ERR_NULL,
    /** Block vector checksum or length/capacity invariant is damaged. */
    WST_LIST_ERR_CORRUPT,
    /** Requested block capacity does not fit into size_t. */
    WST_LIST_ERR_OVERFLOW,
    /** Index is past the end of the list. */
    WST_LIST_ERR_BOUNDS,
} WstListErr;

/** A single untyped node of doubly linked list. */
struct WstListNodeHdr {
    struct WstListNodeHdr *next;
    struct WstListNodeHdr *prev;
};

typedef struct WstListNodeHdr WstListNodeHdr;

/**
 * List header, keeps track of head, tail and linked free nodes
 */
typedef struct {
    /** First live node, `NULL` when the list is empty. */
    WstListNodeHdr *head;
    /** Last live node, `NULL` when the list is empty. */
    WstListNodeHdr *tail;
    /** First free node, free nodes are linked through `next`. */
    WstListNodeHdr *free;
} WstListHdr;

/** Get node at `idx`, or `NULL` when out of bounds. */
WstListNodeHdr *wst_list_hdr_at(const WstListHdr *list, size_t idx);

/** Link `node` after `prev`. `prev == NULL` inserts at the front. */
void wst_list_hdr_insert_after(WstListHdr *list, WstListNodeHdr *prev,
                               WstListNodeHdr *node);

/** Unlink `node` and put it at the front of the free list. */
void wst_list_hdr_remove(WstListHdr *list, WstListNodeHdr *node);

/** Take a node from the free list, or return `NULL` when it is empty. */
WstListNodeHdr *wst_list_hdr_take_free(WstListHdr *list);

/** Reset endpoints to an empty list. */
void wst_list_hdr_reset(WstListHdr *list);

/** Put `count` nodes into the free list, keeping chain valid. */
void wst_list_hdr_add_free(WstListHdr *list, void *nodes, size_t count,
                           size_t stride);

/** Short human-readable description of a list error. */
const char *wst_list_err_str(WstListErr err);

#endif /* TOYS_LIST_INTERNAL_H */

/** Temporary for editor. */
#ifndef LIST_T
#define LIST_T WstListInt
#define LIST_ELEM int
#define LIST_F(it) wst_list_int_ ## it
#define LIST_FMT "%d"
#define LIST_IMPL
#endif

#ifdef LIST_T
#if !defined(LIST_ELEM)
#  error LIST_ELEM must be defined with the name of list element type
#endif

#if !defined(LIST_F)
#  error LIST_F must be defined as function macro that generates names for list impl functions
#endif

#if !defined(LIST_REF) && !defined(LIST_FMT)
#  error LIST_FMT must be defined with list element format string, or define LIST_REF
#endif

/* public func names generation */
#define list_free LIST_F(free)
#define list_clear LIST_F(clear)
#define list_init LIST_F(init)
#define list_verify LIST_F(verify)
#define list_dump LIST_F(dump)

// #define list_reserve LIST_F(reserve)

#define list_get_node LIST_F(get_node)
#define list_get LIST_F(get)
#define list_remove_node LIST_F(remove_node)
#define list_remove LIST_F(remove)
#define list_pop LIST_F(pop)

#define list_find_node LIST_F(find_node)
#define list_find LIST_F(find)
#define list_set LIST_F(set)
#define list_append LIST_F(append)
#define list_insert_by_node LIST_F(insert_by_node)
#define list_insert LIST_F(insert)

/* non-public funcs */
#define list_err_from_vec LIST_F(err_from_vec)
#define list_drop_blocks LIST_F(drop_blocks)
#define list_new_block LIST_F(new_block)
#define list_alloc_node LIST_F(alloc_node)
#define list_insert_after LIST_F(insert_after)
#define list_insert_at LIST_F(insert_at)
#define list_find_value LIST_F(find_value)
#define list_find_index LIST_F(find_index)

// #define list_push_front LIST_F(push_front)
// #define list_pop_front LIST_F(pop_front)

// #define list_ensure LIST_F(ensure)
// #define list_resize LIST_F(resize)
// #define list_seal LIST_F(seal)

#define IMPL2_WstListNode(a, b) a ## b
#define IMPL_WstListNode(a, b) IMPL2_WstListNode(a, b)
#define LIST_NODE_T IMPL_WstListNode(LIST_T, Node)
#define LIST_BLOCK_T IMPL_WstListNode(LIST_T, Block)
#define LIST_BLOCK_PTR_T IMPL_WstListNode(LIST_T, BlockPtr)

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


#define VEC_ELEM LIST_BLOCK_PTR_T
#define VEC_T BLOCK_VEC_T
#define VEC_F(name) BLOCK_VEC_F(name)
#define VEC_REF

#if defined(LIST_IMPL) && !defined(VEC_IMPL)
#  define VEC_IMPL
#  include "toys/vec_internal.h"
#  undef VEC_IMPL
#elif !defined(LIST_IMPL) && defined(VEC_IMPL)
#  undef VEC_IMPL
#  include "toys/vec_internal.h"
#  define VEC_IMPL
#else
#  include "toys/vec_internal.h"
#endif /* LIST_IMPL ^ VEC_IMPL */


/** Doubly linked list. */
typedef struct {
    WstListHdr hdr;
    BLOCK_VEC_T blocks;
} LIST_T;


/** Free list memory and zero endpoints. Refuses on verification failure. */
WstListErr list_free(LIST_T *list);

/** Clear all elements, keep allocation/capacity. Refuses on verification failure. */
WstListErr list_clear(LIST_T *list);

/** Initialize list as empty. */
WstListErr list_init(LIST_T *list);

/** Check the block vector for unexpected corruption. */
WstListErr list_verify(const LIST_T *list);

/** Dump all debug information about list and its contents. */
void list_dump(const LIST_T *list);

/**
 * Get list node by index. Returns `NULL` when not found.
 *
 * Node pointers point into heap blocks and stay valid across insertions.
 * They are invalidated by `list_free` and become stale after `list_clear`.
 */
LIST_NODE_T *list_get_node(const LIST_T *list, size_t idx);

/** Get value by index. Returns `false` when index is out of bounds. */
bool list_get(const LIST_T *list, size_t idx, LIST_ELEM *out);

/** Remove list element by its node pointer, which must belong to `list`. */
bool list_remove_node(LIST_T *list, const LIST_NODE_T *node);

/** Remove list element by index. */
bool list_remove(LIST_T *list, size_t idx);

/** Pop list back (last element). */
WstListErr list_pop(LIST_T *list, LIST_ELEM *out);

#ifdef LIST_REF

/** Find list node by value (bytewise). Returns `NULL` when not found. */
LIST_NODE_T *list_find_node(const LIST_T *list, const LIST_ELEM *val);

/** Find list element index by value (bytewise). Returns `-1` when not found. */
ssize_t list_find(const LIST_T *list, const LIST_ELEM *val);

/** Set list element value by index. Returns `false` when index is out of bounds. */
bool list_set(LIST_T *list, size_t idx, const LIST_ELEM *val);

/** Append value to back (tail) of the list. */
WstListErr list_append(LIST_T *list, const LIST_ELEM *val);

/**
 * Insert a new element after `node`, which must belong to `list`.
 * `NULL` prepends to the list.
 */
WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               const LIST_ELEM *val);

/** Insert a new element at `idx`, shifting elements at and after it back. */
WstListErr list_insert(LIST_T *list, size_t idx, const LIST_ELEM *val);

#else /* !LIST_REF */

/** Find list node by value (bytewise). Returns `NULL` when not found. */
LIST_NODE_T *list_find_node(const LIST_T *list, LIST_ELEM val);

/** Find list element index by value (bytewise). Returns `-1` when not found. */
ssize_t list_find(const LIST_T *list, LIST_ELEM val);

/** Set list element value by index. Returns `false` when index is out of bounds. */
bool list_set(LIST_T *list, size_t idx, LIST_ELEM val);

/** Append value to back (tail) of the list. */
WstListErr list_append(LIST_T *list, LIST_ELEM val);

/**
 * Insert a new element after `node`, which must belong to `list`.
 * `NULL` prepends to the list.
 */
WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               LIST_ELEM val);

/** Insert a new element at `idx`, shifting elements at and after it back. */
WstListErr list_insert(LIST_T *list, size_t idx, LIST_ELEM val);

#endif /* LIST_REF */



#ifdef LIST_IMPL

/** Elements are compared bytewise, as the API takes no comparator. */
#define LIST_EQ(a, b) (memcmp(&(a), (b), sizeof(LIST_ELEM)) == 0)

/** Translate a block-vector error into a list error. */
static WstListErr list_err_from_vec(WstVecErr err)
{
    switch (err) {
    case WST_VEC_NO_ERR:
        return WST_LIST_NO_ERR;
    case WST_VEC_ERR_NOMEM:
        return WST_LIST_ERR_NOMEM;
    case WST_VEC_ERR_EMPTY:
        return WST_LIST_ERR_EMPTY;
    case WST_VEC_ERR_NULL:
        return WST_LIST_ERR_NULL;
    case WST_VEC_ERR_CORRUPT:
        return WST_LIST_ERR_CORRUPT;
    case WST_VEC_ERR_OVERFLOW:
        return WST_LIST_ERR_OVERFLOW;
    }
    return WST_LIST_ERR_CORRUPT;
}

/** Free every node block and the block vector; list must verify. */
static WstListErr list_drop_blocks(LIST_T *list)
{
    assert(list != NULL);

    while (list->blocks.length > 0) {
        LIST_BLOCK_PTR_T block = NULL;
        WstVecErr verr = BLOCK_VEC_F(pop)(&list->blocks, &block);
        if (verr != WST_VEC_NO_ERR)
            return list_err_from_vec(verr);
        free(block);
    }

    return list_err_from_vec(BLOCK_VEC_F(free)(&list->blocks));
}

/** Allocate a new geometrically larger block and thread its nodes. */
static WstListErr list_new_block(LIST_T *list)
{
    assert(list != NULL);

    size_t cap = LIST_BLOCK_INIT_CAP;
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
        return list_err_from_vec(verr);
    }

    wst_list_hdr_add_free(&list->hdr, block->nodes, block->cap,
                          sizeof(LIST_NODE_T));
    return WST_LIST_NO_ERR;
}

/** Take a node from the free list, allocating a block when empty. */
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

/** Insert a copy of `val` after `prev`, `NULL` inserts at the front. */
static WstListErr list_insert_after(LIST_T *list, WstListNodeHdr *prev,
                                    const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    LIST_NODE_T *new_node = NULL;
    WstListErr err = list_alloc_node(list, &new_node);
    if (err != WST_LIST_NO_ERR)
        return err;

    new_node->data = *val;
    wst_list_hdr_insert_after(&list->hdr, prev, (WstListNodeHdr *)new_node);
    return WST_LIST_NO_ERR;
}

/** Insert a copy of `val` at `idx`, shifting the rest back. */
static WstListErr list_insert_at(LIST_T *list, size_t idx, const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    WstListNodeHdr *prev = NULL;
    if (idx > 0) {
        prev = wst_list_hdr_at(&list->hdr, idx - 1);
        if (prev == NULL)
            return WST_LIST_ERR_BOUNDS;
    }

    return list_insert_after(list, prev, val);
}

/** Find node holding `val` without verifying the list. */
static LIST_NODE_T *list_find_value(const LIST_T *list, const LIST_ELEM *val)
{
    for (WstListNodeHdr *hdr = list->hdr.head; hdr != NULL; hdr = hdr->next) {
        LIST_NODE_T *node = (LIST_NODE_T *)hdr;
        if (LIST_EQ(node->data, val))
            return node;
    }
    return NULL;
}

/** Find index of node holding `val` without verifying the list. */
static ssize_t list_find_index(const LIST_T *list, const LIST_ELEM *val)
{
    size_t idx = 0;
    for (WstListNodeHdr *hdr = list->hdr.head; hdr != NULL;
         hdr = hdr->next, idx++) {
        LIST_NODE_T *node = (LIST_NODE_T *)hdr;
        if (LIST_EQ(node->data, val))
            return (ssize_t)idx;
    }
    return -1;
}


WstListErr list_free(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

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
        return list_err_from_vec(verr);

    LOG_D("initialized empty list");
    return WST_LIST_NO_ERR;
}

WstListErr list_verify(const LIST_T *list)
{
    if (list == NULL) {
        LOG_E("cannot verify null list");
        return WST_LIST_ERR_NULL;
    }

    return list_err_from_vec(BLOCK_VEC_F(verify)(&list->blocks));
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

LIST_NODE_T *list_get_node(const LIST_T *list, size_t idx)
{
    assert(list != NULL);

    if (list_verify(list) != WST_LIST_NO_ERR)
        return NULL;

    return (LIST_NODE_T *)wst_list_hdr_at(&list->hdr, idx);
}

bool list_get(const LIST_T *list, size_t idx, LIST_ELEM *out)
{
    assert(out != NULL);

    const LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    *out = node->data;
    return true;
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

bool list_remove(LIST_T *list, size_t idx)
{
    const LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    return list_remove_node(list, node);
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

#ifdef LIST_REF

LIST_NODE_T *list_find_node(const LIST_T *list, const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    if (list_verify(list) != WST_LIST_NO_ERR)
        return NULL;

    return list_find_value(list, val);
}

ssize_t list_find(const LIST_T *list, const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    if (list_verify(list) != WST_LIST_NO_ERR)
        return -1;

    return list_find_index(list, val);
}

bool list_set(LIST_T *list, size_t idx, const LIST_ELEM *val)
{
    assert(val != NULL);

    LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    node->data = *val;
    return true;
}

WstListErr list_append(LIST_T *list, const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_after(list, list->hdr.tail, val);
}

WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_after(list, (WstListNodeHdr *)node, val);
}

WstListErr list_insert(LIST_T *list, size_t idx, const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_at(list, idx, val);
}

#else /* !LIST_REF */

LIST_NODE_T *list_find_node(const LIST_T *list, LIST_ELEM val)
{
    assert(list != NULL);

    if (list_verify(list) != WST_LIST_NO_ERR)
        return NULL;

    return list_find_value(list, &val);
}

ssize_t list_find(const LIST_T *list, LIST_ELEM val)
{
    assert(list != NULL);

    if (list_verify(list) != WST_LIST_NO_ERR)
        return -1;

    return list_find_index(list, &val);
}

bool list_set(LIST_T *list, size_t idx, LIST_ELEM val)
{
    LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    node->data = val;
    return true;
}

WstListErr list_append(LIST_T *list, LIST_ELEM val)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_after(list, list->hdr.tail, &val);
}

WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               LIST_ELEM val)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_after(list, (WstListNodeHdr *)node, &val);
}

WstListErr list_insert(LIST_T *list, size_t idx, LIST_ELEM val)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_at(list, idx, &val);
}

#endif /* LIST_REF */

#undef LIST_EQ

#endif /* LIST_IMPL */

#undef list_free
#undef list_clear
#undef list_init
#undef list_verify
#undef list_dump
// #undef list_reserve
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
// #undef list_push_front
// #undef list_pop_front
// #undef list_ensure
// #undef list_resize
// #undef list_seal

#undef list_err_from_vec
#undef list_drop_blocks
#undef list_new_block
#undef list_alloc_node
#undef list_insert_after
#undef list_insert_at
#undef list_find_value
#undef list_find_index

#undef IMPL2_WstListNode
#undef IMPL_WstListNode
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
