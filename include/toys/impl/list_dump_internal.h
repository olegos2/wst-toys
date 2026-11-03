/*
 * Pointer list dump and graph rendering. Included by list_internal.h
 * inside its implementation block so it can use the stamped types.
 */

#include "toys/debug.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** Number of node slots across all blocks. */
static size_t list_dump_total(const LIST_T *list)
{
    size_t total = 0;
    for (size_t i = 0; i < list->blocks.length; i++) {
        const LIST_BLOCK_T *block = list->blocks.data[i];
        if (block != NULL)
            total += block->cap;
    }
    return total;
}

/** Map a node address to its flat slot number. */
static bool list_dump_slot_of(const LIST_T *list, const void *ptr,
                              size_t *out)
{
    size_t base = 0;

    for (size_t i = 0; i < list->blocks.length; i++) {
        const LIST_BLOCK_T *block = list->blocks.data[i];
        if (block == NULL)
            continue;

        uintptr_t lo = (uintptr_t)&block->nodes[0];
        uintptr_t hi = (uintptr_t)&block->nodes[block->cap];
        uintptr_t addr = (uintptr_t)ptr;

        if (addr >= lo && addr < hi) {
            size_t off = (size_t)(addr - lo);
            if (off % sizeof(LIST_NODE_T) != 0)
                return false;
            *out = base + off / sizeof(LIST_NODE_T);
            return true;
        }
        base += block->cap;
    }

    return false;
}

/** Get a node by its flat slot number. */
static const LIST_NODE_T *list_dump_node_at(const LIST_T *list, size_t slot)
{
    for (size_t i = 0; i < list->blocks.length; i++) {
        const LIST_BLOCK_T *block = list->blocks.data[i];
        if (block == NULL)
            continue;
        if (slot < block->cap)
            return &block->nodes[slot];
        slot -= block->cap;
    }
    return NULL;
}

/** Format a node pointer as a slot number, or a raw pointer if unknown. */
static void list_dump_ref(const LIST_T *list, const void *ptr, char *buf,
                          size_t cap)
{
    size_t slot;

    if (ptr == NULL)
        snprintf(buf, cap, "-");
    else if (list_dump_slot_of(list, ptr, &slot))
        snprintf(buf, cap, "%zu", slot);
    else
        snprintf(buf, cap, "%p", ptr);
}

static void list_dump_mark_live(const LIST_T *list, WstListSlotState *state,
                                size_t total)
{
    const WstListNodeHdr *node = list->hdr.head;
    for (size_t step = 0; node != NULL && step < total; step++) {
        size_t slot;
        if (!list_dump_slot_of(list, node, &slot))
            break;
        if (state[slot] == WST_LIST_SLOT_ORPHAN)
            state[slot] = WST_LIST_SLOT_LIVE;
        node = node->next;
    }
}

static void list_dump_mark_free(const LIST_T *list, WstListSlotState *state,
                                size_t total)
{
    const WstListNodeHdr *node = list->hdr.free;
    for (size_t step = 0; node != NULL && step < total; step++) {
        size_t slot;
        if (!list_dump_slot_of(list, node, &slot))
            break;
        if (state[slot] == WST_LIST_SLOT_ORPHAN)
            state[slot] = WST_LIST_SLOT_FREE;
        node = node->next;
    }
}

/** Follow one chain and report breaks or cycles. */
static void list_dump_check_chain(const LIST_T *list, const void *start,
                                  const char *name, size_t total,
                                  WstListDiag *diag)
{
    const WstListNodeHdr *node = start;
    for (size_t step = 0; node != NULL; step++) {
        size_t slot;
        if (!list_dump_slot_of(list, node, &slot) || slot >= total) {
            wst_list_diag_add(diag, "%s chain leaves node blocks at %p", name,
                              (const void *)node);
            return;
        }
        if (step >= total) {
            wst_list_diag_add(diag, "%s chain does not terminate", name);
            return;
        }
        node = node->next;
    }
}

/** Check one live node's links and both directions of each link. */
static void list_dump_check_live(const LIST_T *list, size_t slot,
                                 const WstListNodeHdr *node,
                                 const WstListSlotState *state, size_t total,
                                 bool head_ok, size_t head_slot,
                                 bool tail_ok, size_t tail_slot,
                                 WstListDiag *diag)
{
    const WstListNodeHdr *links[] = { node->prev, node->next };
    const char *names[] = { "prev", "next" };

    for (int i = 0; i < 2; i++) {
        const WstListNodeHdr *target = links[i];
        if (target == NULL) {
            if (i == 0 && (!head_ok || slot != head_slot))
                wst_list_diag_add(
                    diag, "live slot %zu prev is NULL but is not head", slot);
            if (i == 1 && (!tail_ok || slot != tail_slot))
                wst_list_diag_add(
                    diag, "live slot %zu next is NULL but is not tail", slot);
            continue;
        }

        size_t target_slot;
        if (!list_dump_slot_of(list, target, &target_slot) ||
            target_slot >= total) {
            wst_list_diag_add(
                diag, "live slot %zu %s=%p is outside node blocks", slot,
                names[i], (const void *)target);
            continue;
        }
        if (state[target_slot] != WST_LIST_SLOT_LIVE) {
            wst_list_diag_add(diag, "live slot %zu %s -> %s slot %zu", slot,
                              names[i],
                              wst_list_slot_state_name(state[target_slot]),
                              target_slot);
            continue;
        }

        const LIST_NODE_T *target_node =
            list_dump_node_at(list, target_slot);
        if (target_node == NULL)
            continue;

        const WstListNodeHdr *back =
            i == 0 ? target_node->hdr.next : target_node->hdr.prev;
        if (back != node)
            wst_list_diag_add(diag,
                              "live slot %zu %s=%zu but slot %zu %s=%p",
                              slot, names[i], target_slot, target_slot,
                              i == 0 ? "next" : "prev", (const void *)back);
    }
}

/** Check a free node's links. Free nodes use only next. */
static void list_dump_check_free(const LIST_T *list, size_t slot,
                                 const WstListNodeHdr *node,
                                 const WstListSlotState *state, size_t total,
                                 WstListDiag *diag)
{
    if (node->prev != NULL)
        wst_list_diag_add(diag, "free slot %zu prev=%p should be NULL", slot,
                          (const void *)node->prev);

    if (node->next == NULL)
        return;

    size_t target_slot;
    if (!list_dump_slot_of(list, node->next, &target_slot) ||
        target_slot >= total) {
        wst_list_diag_add(diag,
                          "free slot %zu next=%p is outside node blocks",
                          slot, (const void *)node->next);
    } else if (state[target_slot] != WST_LIST_SLOT_FREE) {
        wst_list_diag_add(diag, "free slot %zu next -> %s slot %zu", slot,
                          wst_list_slot_state_name(state[target_slot]),
                          target_slot);
    }
}

/** Check block vector entries, head/tail/free, chains and every slot. */
static void list_dump_check(const LIST_T *list, const WstListSlotState *state,
                            size_t total, WstListDiag *diag)
{
    if (state == NULL) {
        wst_list_diag_add(diag, "slot state allocation failed");
        return;
    }

    for (size_t i = 0; i < list->blocks.length; i++) {
        const LIST_BLOCK_T *block = list->blocks.data[i];
        if (block == NULL)
            wst_list_diag_add(diag, "blocks[%zu] is NULL", i);
        else if (block->cap == 0)
            wst_list_diag_add(diag, "block[%zu] has cap 0", i);
    }

    size_t head_slot = 0;
    size_t tail_slot = 0;
    size_t free_slot = 0;
    bool head_ok = list->hdr.head != NULL &&
                   list_dump_slot_of(list, list->hdr.head, &head_slot) &&
                   head_slot < total;
    bool tail_ok = list->hdr.tail != NULL &&
                   list_dump_slot_of(list, list->hdr.tail, &tail_slot) &&
                   tail_slot < total;
    bool free_ok = list->hdr.free != NULL &&
                   list_dump_slot_of(list, list->hdr.free, &free_slot) &&
                   free_slot < total;

    if (list->hdr.head != NULL && !head_ok)
        wst_list_diag_add(diag, "head=%p is outside node blocks",
                          (const void *)list->hdr.head);
    if (list->hdr.tail != NULL && !tail_ok)
        wst_list_diag_add(diag, "tail=%p is outside node blocks",
                          (const void *)list->hdr.tail);
    if (list->hdr.free != NULL && !free_ok)
        wst_list_diag_add(diag, "free=%p is outside node blocks",
                          (const void *)list->hdr.free);
    if (list->hdr.head == NULL && list->hdr.tail != NULL)
        wst_list_diag_add(diag, "head is NULL but tail=%p",
                          (const void *)list->hdr.tail);
    if (list->hdr.head != NULL && list->hdr.tail == NULL)
        wst_list_diag_add(diag, "tail is NULL but head=%p",
                          (const void *)list->hdr.head);

    if (head_ok && state[head_slot] != WST_LIST_SLOT_LIVE)
        wst_list_diag_add(diag, "head -> %s slot %zu",
                          wst_list_slot_state_name(state[head_slot]),
                          head_slot);
    if (tail_ok && state[tail_slot] != WST_LIST_SLOT_LIVE)
        wst_list_diag_add(diag, "tail -> %s slot %zu",
                          wst_list_slot_state_name(state[tail_slot]),
                          tail_slot);
    if (free_ok && state[free_slot] != WST_LIST_SLOT_FREE)
        wst_list_diag_add(diag, "free -> %s slot %zu",
                          wst_list_slot_state_name(state[free_slot]),
                          free_slot);

    if (head_ok && list->hdr.head->prev != NULL)
        wst_list_diag_add(diag, "head slot %zu prev=%p should be NULL",
                          head_slot, (const void *)list->hdr.head->prev);
    if (tail_ok && list->hdr.tail->next != NULL)
        wst_list_diag_add(diag, "tail slot %zu next=%p should be NULL",
                          tail_slot, (const void *)list->hdr.tail->next);
    if (free_ok && list->hdr.free->prev != NULL)
        wst_list_diag_add(diag, "free head slot %zu prev=%p should be NULL",
                          free_slot, (const void *)list->hdr.free->prev);

    list_dump_check_chain(list, list->hdr.head, "live", total, diag);
    list_dump_check_chain(list, list->hdr.free, "free", total, diag);

    for (size_t slot = 0; slot < total; slot++) {
        const LIST_NODE_T *node = list_dump_node_at(list, slot);
        if (node == NULL) {
            wst_list_diag_add(diag, "slot %zu has no block", slot);
            continue;
        }

        switch (state[slot]) {
        case WST_LIST_SLOT_LIVE:
            list_dump_check_live(list, slot, &node->hdr, state, total,
                                 head_ok, head_slot, tail_ok, tail_slot,
                                 diag);
            break;
        case WST_LIST_SLOT_FREE:
            list_dump_check_free(list, slot, &node->hdr, state, total, diag);
            break;
        case WST_LIST_SLOT_ORPHAN:
        default:
            wst_list_diag_add(diag, "slot %zu is neither live nor free", slot);
            break;
        }
    }
}

static void list_dump_graph(const LIST_T *list, size_t issues)
{
    if (!wst_log_accepts_images())
        return;

    size_t total = list_dump_total(list);
    WstListSlotState *state =
        calloc(total > 0 ? total : 1, sizeof(*state));
    if (state == NULL)
        return;

    list_dump_mark_live(list, state, total);
    list_dump_mark_free(list, state, total);
    WstListSlotCounts counts = wst_list_slot_counts(state, total);

    char free_ref[32];
    list_dump_ref(list, list->hdr.free, free_ref, sizeof(free_ref));

    WstListGraph *graph = wst_list_graph_new(
        WST_LIST_STR(LIST_T),
        "blocks=%zu\nslots=%zu\nlive=%zu\npooled=%zu\n"
        "orphan=%zu\nfree=%s\nissues=%zu",
        list->blocks.length, total, counts.live, counts.pooled,
        counts.orphan, free_ref, issues);
    if (graph == NULL) {
        free(state);
        return;
    }

    size_t base = 0;
    for (size_t b = 0; b < list->blocks.length; b++) {
        const LIST_BLOCK_T *block = list->blocks.data[b];
        if (block == NULL)
            continue;

        wst_list_graph_block_begin(graph, b, block->cap, block);
        for (size_t j = 0; j < block->cap; j++) {
            size_t slot = base + j;
            const LIST_NODE_T *node = &block->nodes[j];
            WstListSlotState node_state = state[slot];

            char prev_ref[32];
            char next_ref[32];
            list_dump_ref(list, node->hdr.prev, prev_ref, sizeof(prev_ref));
            list_dump_ref(list, node->hdr.next, next_ref, sizeof(next_ref));

#ifdef LIST_FMT
            wst_list_graph_node(graph, slot, node_state,
                                "#%zu\nprev=%s\nnext=%s\ndata=" LIST_FMT,
                                slot, prev_ref, next_ref, node->data);
#else
            wst_list_graph_node(graph, slot, node_state,
                                "#%zu\nprev=%s\nnext=%s\ndata=%p", slot,
                                prev_ref, next_ref, (const void *)&node->data);
#endif
        }
        wst_list_graph_block_end(graph);
        base += block->cap;
    }

    /* Define every node before its edges so cross-block links do not create
       the target inside the previous block's cluster. */
    base = 0;
    for (size_t b = 0; b < list->blocks.length; b++) {
        const LIST_BLOCK_T *block = list->blocks.data[b];
        if (block == NULL)
            continue;

        wst_list_graph_block_begin(graph, b, block->cap, block);
        for (size_t j = 0; j < block->cap; j++) {
            size_t slot = base + j;
            const LIST_NODE_T *node = &block->nodes[j];
            WstListSlotState node_state = state[slot];
            size_t target;

            if (node->hdr.next != NULL) {
                WstListGraphEdge kind = WST_LIST_GRAPH_EDGE_BAD;
                if (list_dump_slot_of(list, node->hdr.next, &target) &&
                    target < total) {
                    if (node_state == WST_LIST_SLOT_FREE &&
                        state[target] == WST_LIST_SLOT_FREE)
                        kind = WST_LIST_GRAPH_EDGE_FREE_NEXT;
                    else if (node_state == WST_LIST_SLOT_LIVE &&
                             state[target] == WST_LIST_SLOT_LIVE)
                        kind = WST_LIST_GRAPH_EDGE_NEXT;
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_slot(target), kind);
                } else {
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_null(),
                                        WST_LIST_GRAPH_EDGE_BAD);
                }
            }
            if (node->hdr.prev != NULL) {
                WstListGraphEdge kind = WST_LIST_GRAPH_EDGE_BAD;
                if (node_state == WST_LIST_SLOT_LIVE &&
                    list_dump_slot_of(list, node->hdr.prev, &target) &&
                    target < total && state[target] == WST_LIST_SLOT_LIVE)
                    kind = WST_LIST_GRAPH_EDGE_PREV;

                if (list_dump_slot_of(list, node->hdr.prev, &target) &&
                    target < total)
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_slot(target), kind);
                else
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_null(),
                                        WST_LIST_GRAPH_EDGE_BAD);
            }
        }
        wst_list_graph_block_end(graph);
        base += block->cap;
    }

    WstListGraphRef header = wst_list_graph_ref_header();
    WstListGraphRef null_ref = wst_list_graph_ref_null();
    size_t target;

    if (list->hdr.head == NULL) {
        wst_list_graph_edge(graph, header, null_ref, WST_LIST_GRAPH_EDGE_HEAD);
    } else if (list_dump_slot_of(list, list->hdr.head, &target) &&
               target < total) {
        WstListGraphEdge kind = state[target] == WST_LIST_SLOT_LIVE
                                    ? WST_LIST_GRAPH_EDGE_HEAD
                                    : WST_LIST_GRAPH_EDGE_BAD_HEAD;
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(target),
                            kind);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_HEAD);
    }

    if (list->hdr.tail == NULL) {
        wst_list_graph_edge(graph, header, null_ref, WST_LIST_GRAPH_EDGE_TAIL);
    } else if (list_dump_slot_of(list, list->hdr.tail, &target) &&
               target < total) {
        WstListGraphEdge kind = state[target] == WST_LIST_SLOT_LIVE
                                    ? WST_LIST_GRAPH_EDGE_TAIL
                                    : WST_LIST_GRAPH_EDGE_BAD_TAIL;
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(target),
                            kind);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_TAIL);
    }

    if (list->hdr.free == NULL) {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_FREE_HEAD);
    } else if (list_dump_slot_of(list, list->hdr.free, &target) &&
               target < total) {
        WstListGraphEdge kind = state[target] == WST_LIST_SLOT_FREE
                                    ? WST_LIST_GRAPH_EDGE_FREE_HEAD
                                    : WST_LIST_GRAPH_EDGE_BAD_FREE;
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(target),
                            kind);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_FREE);
    }

    wst_list_graph_emit(graph, "linked list graph");
    wst_list_graph_free(graph);
    free(state);
}
