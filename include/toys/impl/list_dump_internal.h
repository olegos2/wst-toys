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

static void list_dump_graph(const LIST_T *list)
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
        "blocks=%zu slots=%zu live=%zu pooled=%zu orphan=%zu free=%s",
        list->blocks.length, total, counts.live, counts.pooled,
        counts.orphan, free_ref);
    if (graph == NULL) {
        free(state);
        return;
    }

    size_t base = 0;
    for (size_t b = 0; b < list->blocks.length; b++) {
        const LIST_BLOCK_T *block = list->blocks.data[b];
        if (block == NULL)
            continue;

        bool visible = false;
        for (size_t j = 0; j < block->cap; j++) {
            if (state[base + j] != WST_LIST_SLOT_FREE) {
                visible = true;
                break;
            }
        }
        if (!visible) {
            base += block->cap;
            continue;
        }

        wst_list_graph_block_begin(graph, b, block->cap, block);
        for (size_t j = 0; j < block->cap; j++) {
            size_t slot = base + j;
            const LIST_NODE_T *node = &block->nodes[j];
            WstListSlotState node_state = state[slot];
            if (node_state == WST_LIST_SLOT_FREE)
                continue;

            char prev_ref[32];
            char next_ref[32];
            list_dump_ref(list, node->hdr.prev, prev_ref, sizeof(prev_ref));
            list_dump_ref(list, node->hdr.next, next_ref, sizeof(next_ref));

#ifdef LIST_FMT
            wst_list_graph_node(graph, slot, node_state,
                                "#%zu\nprev=%s\tnext=%s\ndata=" LIST_FMT,
                                slot, prev_ref, next_ref, node->data);
#else
            wst_list_graph_node(graph, slot, node_state,
                                "#%zu\nprev=%s\tnext=%s\ndata@%p", slot,
                                prev_ref, next_ref, (const void *)&node->data);
#endif

            size_t target;
            if (node->hdr.next != NULL) {
                if (list_dump_slot_of(list, node->hdr.next, &target) &&
                    target < total && state[target] != WST_LIST_SLOT_FREE) {
                    WstListGraphEdge kind =
                        node_state == WST_LIST_SLOT_LIVE
                            ? WST_LIST_GRAPH_EDGE_NEXT
                            : WST_LIST_GRAPH_EDGE_BAD;
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_slot(target), kind);
                } else {
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_null(),
                                        WST_LIST_GRAPH_EDGE_BAD);
                }
            }
            if (node->hdr.prev != NULL) {
                if (list_dump_slot_of(list, node->hdr.prev, &target) &&
                    target < total && state[target] != WST_LIST_SLOT_FREE) {
                    WstListGraphEdge kind =
                        node_state == WST_LIST_SLOT_LIVE
                            ? WST_LIST_GRAPH_EDGE_PREV
                            : WST_LIST_GRAPH_EDGE_BAD;
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_slot(target), kind);
                } else {
                    wst_list_graph_edge(graph, wst_list_graph_ref_slot(slot),
                                        wst_list_graph_ref_null(),
                                        WST_LIST_GRAPH_EDGE_BAD);
                }
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
               target < total && state[target] != WST_LIST_SLOT_FREE) {
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(target),
                            WST_LIST_GRAPH_EDGE_HEAD);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_HEAD);
    }

    if (list->hdr.tail == NULL) {
        wst_list_graph_edge(graph, header, null_ref, WST_LIST_GRAPH_EDGE_TAIL);
    } else if (list_dump_slot_of(list, list->hdr.tail, &target) &&
               target < total && state[target] != WST_LIST_SLOT_FREE) {
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(target),
                            WST_LIST_GRAPH_EDGE_TAIL);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_TAIL);
    }

    wst_list_graph_emit(graph, "linked list graph");
    wst_list_graph_free(graph);
    free(state);
}
