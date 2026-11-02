/*
 * Index list dump and graph rendering. Included by list_idx_internal.h
 * inside its implementation block so it can use the stamped types.
 */

#include "toys/debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Number of slots safe to inspect, limited by the allocation. */
static uint32_t list_dump_len(const LIST_T *list)
{
    if (list->hdr.nodes == NULL)
        return 0;

    uint32_t len = list->hdr.len;
    if ((size_t)len > list->hdr.cap)
        len = (uint32_t)list->hdr.cap;
    return len;
}

static void list_dump_states(const LIST_T *list, WstListSlotState *state,
                             uint32_t len)
{
    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    if (nodes == NULL)
        return;

    uint32_t link = nodes[0].hdr.next;
    for (uint32_t step = 0; link != 0 && link <= len && step <= len; step++) {
        if (state[link] == WST_LIST_SLOT_ORPHAN)
            state[link] = WST_LIST_SLOT_LIVE;
        link = nodes[link].hdr.next;
    }

    link = list->hdr.free;
    for (uint32_t step = 0; link != 0 && link <= len && step <= len; step++) {
        if (state[link] == WST_LIST_SLOT_ORPHAN)
            state[link] = WST_LIST_SLOT_FREE;
        link = nodes[link].hdr.next;
    }
}

static void list_dump_graph(const LIST_T *list)
{
    if (!wst_log_accepts_images())
        return;
    if (list->hdr.nodes == NULL || list->hdr.cap == 0)
        return;

    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    uint32_t len = list_dump_len(list);
    WstListSlotState *state =
        calloc((size_t)len + 1, sizeof(*state));
    if (state == NULL)
        return;

    list_dump_states(list, state, len);
    WstListSlotCounts counts = wst_list_slot_counts(state + 1, len);

    WstListGraph *graph = wst_list_graph_new(
        WST_LIST_STR(LIST_T),
        "len\t%u\ncap\t%zu\nlive\t%zu\npooled\t%zu\n"
        "orphan\t%zu\nfree\t%u",
        len, list->hdr.cap, counts.live, counts.pooled, counts.orphan,
        list->hdr.free);
    if (graph == NULL) {
        free(state);
        return;
    }

    WstListGraphRef header = wst_list_graph_ref_header();
    WstListGraphRef null_ref = wst_list_graph_ref_null();

    for (uint32_t i = 1; i <= len; i++) {
        const LIST_NODE_T *node = &nodes[i];
        WstListSlotState node_state = state[i];
        if (node_state == WST_LIST_SLOT_FREE)
            continue;

#ifdef LIST_FMT
        wst_list_graph_node(graph, i, node_state,
                            "#%u\nprev\t%u\nnext\t%u\ndata\t" LIST_FMT, i,
                            node->hdr.prev, node->hdr.next, node->data);
#else
        wst_list_graph_node(graph, i, node_state,
                            "#%u\nprev\t%u\nnext\t%u\ndata\t%p", i,
                            node->hdr.prev, node->hdr.next,
                            (const void *)&node->data);
#endif

        uint32_t target = node->hdr.next;
        if (target != 0) {
            if (target <= len && state[target] != WST_LIST_SLOT_FREE) {
                WstListGraphEdge kind = node_state == WST_LIST_SLOT_LIVE
                                            ? WST_LIST_GRAPH_EDGE_NEXT
                                            : WST_LIST_GRAPH_EDGE_BAD;
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    wst_list_graph_ref_slot(target), kind);
            } else {
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    null_ref, WST_LIST_GRAPH_EDGE_BAD);
            }
        }

        target = node->hdr.prev;
        if (target != 0) {
            if (target <= len && state[target] != WST_LIST_SLOT_FREE) {
                WstListGraphEdge kind = node_state == WST_LIST_SLOT_LIVE
                                            ? WST_LIST_GRAPH_EDGE_PREV
                                            : WST_LIST_GRAPH_EDGE_BAD;
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    wst_list_graph_ref_slot(target), kind);
            } else {
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    null_ref, WST_LIST_GRAPH_EDGE_BAD);
            }
        }
    }

    uint32_t head = nodes[0].hdr.next;
    uint32_t tail = nodes[0].hdr.prev;

    if (head == 0) {
        wst_list_graph_edge(graph, header, null_ref, WST_LIST_GRAPH_EDGE_HEAD);
    } else if (head <= len && state[head] != WST_LIST_SLOT_FREE) {
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(head),
                            WST_LIST_GRAPH_EDGE_HEAD);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_HEAD);
    }

    if (tail == 0) {
        wst_list_graph_edge(graph, header, null_ref, WST_LIST_GRAPH_EDGE_TAIL);
    } else if (tail <= len && state[tail] != WST_LIST_SLOT_FREE) {
        wst_list_graph_edge(graph, header, wst_list_graph_ref_slot(tail),
                            WST_LIST_GRAPH_EDGE_TAIL);
    } else {
        wst_list_graph_edge(graph, header, null_ref,
                            WST_LIST_GRAPH_EDGE_BAD_TAIL);
    }

    wst_list_graph_emit(graph, "linked list graph");
    wst_list_graph_free(graph);
    free(state);
}
