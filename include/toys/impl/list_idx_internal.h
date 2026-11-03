/**
 * Flat-array list backend using 32-bit index links.
 *
 * Nodes live in one array and `next`/`prev` are 0-based `uint32_t` links.
 * Slot 0 is a special node linking the head and tail, so insertion
 * and removal do not need head/tail edge cases.
 *
 * Public node pointers point into the array and become invalid when a later
 * insertion grows it. Fetch the node again after growth.
 *
 * See `list_common_internal.h` for the stamp macro contract.
 */

#include "list_common_internal.h"


#ifndef TOYS_LIST_IDX_INTERNAL_H
#define TOYS_LIST_IDX_INTERNAL_H

/** Capacity of the first allocation, excluding the sentinel slot. */
#define WST_LISTI_INIT_CAP 8

/** Null link. Slot 0 is the sentinel and is never a real node. */
#define WST_LISTI_NIL 0u

#endif /* TOYS_LIST_IDX_INTERNAL_H */


#ifdef LIST_T

/* Backend-specific helpers. */
#define list_insert_after_link LIST_F(insert_after_link)
#define list_dump_len LIST_F(dump_len)
#define list_dump_states LIST_F(dump_states)
#define list_dump_check_chain LIST_F(dump_check_chain)
#define list_dump_check_live LIST_F(dump_check_live)
#define list_dump_check_free LIST_F(dump_check_free)
#define list_dump_check LIST_F(dump_check)
#define list_dump_graph LIST_F(dump_graph)

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


#ifdef LIST_IMPL

#include "list_graph_internal.h"
#include "toys/debug.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

#include "list_idx_dump_internal.h"

#endif /* LIST_IMPL */

#include "list_api_internal.h"

/* Storage-specific public API. */
#ifdef LIST_IMPL

WstListErr list_free(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = WST_LIST_CHECK(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    wst_listi_hdr_free(&list->hdr);
    return WST_LIST_NO_ERR;
}

WstListErr list_clear(LIST_T *list)
{
    assert(list != NULL);

    WstListErr err = WST_LIST_CHECK(list);
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

WstListErr list_reserve(LIST_T *list, size_t count)
{
    assert(list != NULL);

    WstListErr err = WST_LIST_CHECK(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return wst_listi_hdr_reserve(&list->hdr, sizeof(LIST_NODE_T), count);
}

WstListErr list_verify(const LIST_T *list)
{
    if (list == NULL)
        return WST_LIST_ERR_NULL;

    return wst_listi_hdr_verify(&list->hdr, sizeof(LIST_NODE_T));
}

void list_dump(const LIST_T *list)
{
    if (list == NULL) {
        LOG_E("list: <null>");
        return;
    }

    WstListErr verr = list_verify(list);
    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    uint32_t len = list_dump_len(list);
    bool has_sentinel = nodes != NULL && list->hdr.cap > 0;

    uint32_t head = WST_LISTI_NIL;
    uint32_t tail = WST_LISTI_NIL;
    if (has_sentinel) {
        head = nodes[0].hdr.next;
        tail = nodes[0].hdr.prev;
    }

    WstListSlotState *state = NULL;
    WstListSlotCounts counts = { 0 };

    if (has_sentinel) {
        state = calloc((size_t)len + 1, sizeof(*state));
        if (state != NULL) {
            list_dump_states(list, state, len);
            counts = wst_list_slot_counts(state + 1, len);
        }
    }

    char *issue_buf = NULL;
    size_t issue_len = 0;
    FILE *issues = open_memstream(&issue_buf, &issue_len);
    WstListDiag diag = { issues, 0 };

    if (verr != WST_LIST_NO_ERR)
        wst_list_diag_add(&diag, "header check failed: %s",
                          wst_list_err_str(verr));
    if (has_sentinel)
        list_dump_check(list, state, len, &diag);

    if (issues != NULL)
        fclose(issues);

    char *buf = NULL;
    size_t buf_len = 0;
    FILE *out = open_memstream(&buf, &buf_len);
    if (out == NULL) {
        LOG_E("list: cannot allocate dump buffer");
        free(issue_buf);
        free(state);
        return;
    }

#define P(...) fprintf(out, __VA_ARGS__)
    P("list %s @%p\n", WST_LIST_STR(LIST_T), (const void *)list);
    P("  %-8s %s\n", "header",
      verr == WST_LIST_NO_ERR ? "ok" : wst_list_err_str(verr));
    P("  %-8s %u\n", "head", head);
    P("  %-8s %u\n", "tail", tail);
    P("  %-8s %u\n", "free", list->hdr.free);
    P("  %-8s %u\n", "len", list->hdr.len);
    P("  %-8s %zu\n", "cap", list->hdr.cap);

    if (state != NULL) {
        P("  %-8s %zu\n", "live", counts.live);
        P("  %-8s %zu\n", "pooled", counts.pooled);
        P("  %-8s %zu\n", "orphan", counts.orphan);
    }

    if (diag.count > 0 && issue_buf != NULL) {
        P("\ncorruption (%zu)\n", diag.count);
        P("%s", issue_buf);
    }

    if (!has_sentinel) {
        P("\nraw slots omitted, node array is null\n");
    } else if (len == 0) {
        P("\nraw slots empty\n");
    } else {
        P("\n  idx  state     next  prev  data\n");

        for (uint32_t i = 1; i <= len; i++) {
            const LIST_NODE_T *node = &nodes[i];
            WstListSlotState slot_state =
                state != NULL ? state[i] : WST_LIST_SLOT_ORPHAN;
            P("  [%3u] %-6s  %4u  %4u", i,
              wst_list_slot_state_name(slot_state), node->hdr.next,
              node->hdr.prev);
            if (slot_state == WST_LIST_SLOT_FREE) {
                P("  -");
            } else {
#ifdef LIST_FMT
                P("  " LIST_FMT, node->data);
#else
                P("  data@%p", (const void *)&node->data);
#endif
            }
            fputc('\n', out);
        }

        if ((size_t)list->hdr.len > list->hdr.cap) {
            P("\nraw slots truncated at cap %zu, header len=%u\n",
              list->hdr.cap, list->hdr.len);
        }
    }
#undef P

    fclose(out);
    wst_log_print(WST_LOG_INFO, "%s", buf);
    free(buf);
    free(issue_buf);
    free(state);

    if (has_sentinel)
        list_dump_graph(list, diag.count);
}

bool list_remove_node(LIST_T *list, const LIST_NODE_T *node)
{
    assert(list != NULL);

    if (node == NULL)
        return false;

    if (WST_LIST_CHECK(list) != WST_LIST_NO_ERR || list->hdr.len == 0)
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

    WstListErr err = WST_LIST_CHECK(list);
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
#undef list_reserve
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
#undef list_dump_len
#undef list_dump_states
#undef list_dump_check_chain
#undef list_dump_check_live
#undef list_dump_check_free
#undef list_dump_check
#undef list_dump_graph
#undef list_node_at_raw
#undef list_find_node_raw
#undef list_find_index_raw
#undef list_append_raw
#undef list_insert_after_node_raw

#undef WST_LIST_CHECK

#undef LIST_NODES
#undef LIST_NODE_T

#undef LIST_REF
#undef LIST_FMT
#undef LIST_F
#undef LIST_ELEM
#undef LIST_T

#endif /* LIST_T */
