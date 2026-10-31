/**
 * Flat-array list backend using 32-bit index links.
 *
 * Nodes live in one array and `next`/`prev` are 0-based `uint32_t` links.
 * Slot 0 is a sentinel pseudo-node linking the head and tail, so insertion
 * and removal need no head/tail special cases.
 *
 * Public node pointers point into the array and become invalid when a later
 * insertion grows it. Fetch the node again after growth.
 *
 * See `list_internal.h` for the pointer/block backend and
 * `list_common_internal.h` for the stamp macro contract.
 */

#include "list_common_internal.h"


/*
 * Only the constants are guarded. The stamp below expands again for each
 * LIST_T include.
 */
#ifndef TOYS_LIST_IDX_INTERNAL_H
#define TOYS_LIST_IDX_INTERNAL_H

/** Capacity of the first allocation, excluding the sentinel slot. */
#define WST_LISTI_INIT_CAP 8

/** Null link. Slot 0 is the sentinel and is never a real node. */
#define WST_LISTI_NIL 0u

#endif /* TOYS_LIST_IDX_INTERNAL_H */

#ifdef LIST_T

/* Backend-specific helpers, kept in the generated namespace. */
#define list_insert_after_link LIST_F(insert_after_link)

/** Typed view of the untyped node array. */
#define LIST_NODES(list) ((LIST_NODE_T *)(list)->hdr.nodes)

/** A list node whose links are 0-based indices, where 0 is the sentinel. */
typedef struct {
    WstListiNodeHdr hdr;
    LIST_ELEM data;
} LIST_NODE_T;

typedef struct {
    WstListiHdr hdr;
} LIST_T;

/*
 * Raw primitives consumed by the shared typed API. They precede the API
 * include because its common wrappers call them.
 */
#ifdef LIST_IMPL

#include <assert.h>
#include <stdio.h>

/** Get a node without verifying the list. */
static LIST_NODE_T *list_node_at_raw(const LIST_T *list, size_t idx)
{
    assert(list != NULL);
    return (LIST_NODE_T *)wst_listi_hdr_at(&list->hdr, sizeof(LIST_NODE_T),
                                            idx);
}

/** Find the first node holding `val`, without verifying the list. */
static LIST_NODE_T *list_find_node_raw(const LIST_T *list,
                                       const LIST_ELEM *val)
{
    return (LIST_NODE_T *)wst_listi_hdr_find(
        &list->hdr, sizeof(LIST_NODE_T), offsetof(LIST_NODE_T, data), val,
        sizeof(LIST_ELEM));
}

/** Find the first matching index, without verifying the list. */
static ssize_t list_find_index_raw(const LIST_T *list, const LIST_ELEM *val)
{
    return wst_listi_hdr_find_index(
        &list->hdr, sizeof(LIST_NODE_T), offsetof(LIST_NODE_T, data), val,
        sizeof(LIST_ELEM));
}

static WstListErr list_insert_after_link(LIST_T *list, uint32_t prev,
                                         const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    uint32_t link = WST_LISTI_NIL;
    WstListErr err = wst_listi_hdr_alloc(
        &list->hdr, sizeof(LIST_NODE_T), WST_LISTI_INIT_CAP, &link);
    if (err != WST_LIST_NO_ERR)
        return err;

    LIST_NODES(list)[link].data = *val;
    wst_listi_hdr_insert_after(&list->hdr, sizeof(LIST_NODE_T), prev,
                               link);
    return WST_LIST_NO_ERR;
}

/** Insert a copy of `val` after node `prev`, where `NULL` prepends. */
static WstListErr list_insert_after_node_raw(LIST_T *list,
                                             const LIST_NODE_T *prev,
                                             const LIST_ELEM *val)
{
    assert(list != NULL);
    assert(val != NULL);

    uint32_t prev_link = WST_LISTI_NIL;
    if (prev != NULL) {
        if (list->hdr.len == 0)
            return WST_LIST_ERR_BOUNDS;
        prev_link = wst_listi_hdr_index_of(&list->hdr, sizeof(LIST_NODE_T),
                                           &prev->hdr);
    }
    return list_insert_after_link(list, prev_link, val);
}

/** Append a copy of `val` after the current tail. */
static WstListErr list_append_raw(LIST_T *list, const LIST_ELEM *val)
{
    uint32_t prev = wst_listi_hdr_tail(&list->hdr, sizeof(LIST_NODE_T));
    return list_insert_after_link(list, prev, val);
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

    wst_listi_hdr_free(&list->hdr);
    return WST_LIST_NO_ERR;
}

WstListErr list_clear(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    if (list->hdr.len == 0)
        return WST_LIST_NO_ERR;

    wst_listi_hdr_reset(&list->hdr, sizeof(LIST_NODE_T));
    wst_listi_hdr_add_free(&list->hdr, sizeof(LIST_NODE_T), 1,
                           list->hdr.len);
    return WST_LIST_NO_ERR;
}

WstListErr list_init(LIST_T *list)
{
    return list_free(list);
}

WstListErr list_verify(const LIST_T *list)
{
    if (list == NULL)
        return WST_LIST_ERR_NULL;

    return wst_listi_hdr_verify(&list->hdr, sizeof(LIST_NODE_T));
}

void list_dump(const LIST_T *list)
{
    uint32_t head = WST_LISTI_NIL;
    uint32_t tail = WST_LISTI_NIL;
    if (list != NULL && list->hdr.cap > 0 && list->hdr.len > 0 &&
        list->hdr.nodes != NULL) {
        head = wst_listi_hdr_head(&list->hdr, sizeof(LIST_NODE_T));
        tail = wst_listi_hdr_tail(&list->hdr, sizeof(LIST_NODE_T));
    }

    fprintf(stderr, "list <%p>: head=%u tail=%u free=%u len=%u cap=%zu\n",
            (const void *)list, head, tail,
            list == NULL ? WST_LISTI_NIL : list->hdr.free,
            list == NULL ? 0 : list->hdr.len,
            list == NULL ? 0 : list->hdr.cap);

    if (list == NULL)
        return;

    if (list_verify(list) != WST_LIST_NO_ERR) {
        fprintf(stderr, "  raw nodes: omitted due to verification failure\n");
        return;
    }

    if (list->hdr.len == 0)
        return;

    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    fprintf(stderr, "  [   0] sentinel next=%u prev=%u\n",
            nodes[0].hdr.next, nodes[0].hdr.prev);

    for (uint32_t i = 0; i < list->hdr.len; i++) {
        const LIST_NODE_T *node = &nodes[i + 1];
        fprintf(stderr, "  [%4u] next=%u prev=%u",
                i + 1, node->hdr.next, node->hdr.prev);
#ifdef LIST_FMT
        fprintf(stderr, " data=" LIST_FMT, node->data);
#endif
        fprintf(stderr, "\n");
    }
}

bool list_remove_node(LIST_T *list, const LIST_NODE_T *node)
{
    assert(list != NULL);

    if (node == NULL)
        return false;

    if (list_verify(list) != WST_LIST_NO_ERR || list->hdr.len == 0)
        return false;

    uint32_t link = wst_listi_hdr_index_of(
        &list->hdr, sizeof(LIST_NODE_T), &node->hdr);
    if (link == WST_LISTI_NIL)
        return false;

    wst_listi_hdr_remove(&list->hdr, sizeof(LIST_NODE_T), link);
    return true;
}

WstListErr list_pop(LIST_T *list, LIST_ELEM *out)
{
    assert(list != NULL);

    WstListErr err = list_verify(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    uint32_t tail = wst_listi_hdr_tail(&list->hdr, sizeof(LIST_NODE_T));
    if (tail == WST_LISTI_NIL)
        return WST_LIST_ERR_EMPTY;

    if (out != NULL)
        *out = LIST_NODES(list)[tail].data;

    wst_listi_hdr_remove(&list->hdr, sizeof(LIST_NODE_T), tail);
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

#undef list_insert_after_link
#undef list_node_at_raw
#undef list_find_node_raw
#undef list_find_index_raw
#undef list_append_raw
#undef list_insert_after_node_raw

#undef LIST_NODES
#undef LIST_NODE_T

#undef LIST_REF
#undef LIST_FMT
#undef LIST_F
#undef LIST_ELEM
#undef LIST_T

#endif /* LIST_T */
