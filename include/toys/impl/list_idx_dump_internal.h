/*
 * Index list dump and graph rendering. Included by list_idx_internal.h
 * inside its implementation block so it can use the stamped types.
 */

#include "toys/debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

/** Follow one chain and report breaks or cycles. */
static void list_dump_check_chain(const LIST_NODE_T *nodes, uint32_t start,
                                  uint32_t len, const char *name,
                                  WstListDiag *diag)
{
    uint32_t link = start;
    for (uint32_t step = 0; link != 0; step++) {
        if (link > len) {
            wst_list_diag_add(diag, "%s chain leaves node array at %u", name,
                              link);
            return;
        }
        if (step >= len) {
            wst_list_diag_add(diag, "%s chain does not terminate", name);
            return;
        }
        link = nodes[link].hdr.next;
    }
}

/** Check one live node's links and both directions of each link. */
static void list_dump_check_live(const LIST_NODE_T *nodes, uint32_t len,
                                 uint32_t slot, uint32_t head, uint32_t tail,
                                 const WstListSlotState *state,
                                 WstListDiag *diag)
{
    uint32_t links[] = { nodes[slot].hdr.prev, nodes[slot].hdr.next };
    const char *names[] = { "prev", "next" };
    uint32_t ends[] = { head, tail };

    for (int i = 0; i < 2; i++) {
        uint32_t link = links[i];
        if (link == 0) {
            if (slot != ends[i])
                wst_list_diag_add(diag, "live slot %u %s=0 but is not %s",
                                  slot, names[i], i == 0 ? "head" : "tail");
            continue;
        }
        if (link > len) {
            wst_list_diag_add(diag, "live slot %u %s=%u is outside node array",
                              slot, names[i], link);
            continue;
        }
        if (state[link] != WST_LIST_SLOT_LIVE) {
            wst_list_diag_add(diag, "live slot %u %s -> %s slot %u", slot,
                              names[i],
                              wst_list_slot_state_name(state[link]), link);
            continue;
        }

        uint32_t back =
            i == 0 ? nodes[link].hdr.next : nodes[link].hdr.prev;
        if (back != slot)
            wst_list_diag_add(diag,
                              "live slot %u %s=%u but slot %u %s=%u", slot,
                              names[i], link, link,
                              i == 0 ? "next" : "prev", back);
    }
}

/** Check a free node's links. Free nodes use only next. */
static void list_dump_check_free(const LIST_NODE_T *nodes, uint32_t len,
                                 uint32_t slot,
                                 const WstListSlotState *state,
                                 WstListDiag *diag)
{
    if (nodes[slot].hdr.prev != 0)
        wst_list_diag_add(diag, "free slot %u prev=%u should be 0", slot,
                          nodes[slot].hdr.prev);

    uint32_t next = nodes[slot].hdr.next;
    if (next == 0)
        return;

    if (next > len) {
        wst_list_diag_add(diag,
                          "free slot %u next=%u is outside node array", slot,
                          next);
    } else if (state[next] != WST_LIST_SLOT_FREE) {
        wst_list_diag_add(diag, "free slot %u next -> %s slot %u", slot,
                          wst_list_slot_state_name(state[next]), next);
    }
}

/** Check sentinel links, chains and every allocated slot. */
static void list_dump_check(const LIST_T *list, const WstListSlotState *state,
                            uint32_t len, WstListDiag *diag)
{
    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    if (nodes == NULL) {
        wst_list_diag_add(diag, "node array is NULL");
        return;
    }
    if (state == NULL) {
        wst_list_diag_add(diag, "slot state allocation failed");
        return;
    }

    uint32_t head = nodes[0].hdr.next;
    uint32_t tail = nodes[0].hdr.prev;
    uint32_t free = list->hdr.free;

    if (head > len)
        wst_list_diag_add(diag, "head=%u is past len %u", head, len);
    if (tail > len)
        wst_list_diag_add(diag, "tail=%u is past len %u", tail, len);
    if (free > len)
        wst_list_diag_add(diag, "free=%u is past len %u", free, len);
    if ((head == 0) != (tail == 0))
        wst_list_diag_add(diag, "head=%u but tail=%u", head, tail);

    if (head != 0 && head <= len && state[head] != WST_LIST_SLOT_LIVE)
        wst_list_diag_add(diag, "head -> %s slot %u",
                          wst_list_slot_state_name(state[head]), head);
    if (tail != 0 && tail <= len && state[tail] != WST_LIST_SLOT_LIVE)
        wst_list_diag_add(diag, "tail -> %s slot %u",
                          wst_list_slot_state_name(state[tail]), tail);
    if (free != 0 && free <= len && state[free] != WST_LIST_SLOT_FREE)
        wst_list_diag_add(diag, "free -> %s slot %u",
                          wst_list_slot_state_name(state[free]), free);

    list_dump_check_chain(nodes, head, len, "live", diag);
    list_dump_check_chain(nodes, free, len, "free", diag);

    for (uint32_t slot = 1; slot <= len; slot++) {
        switch (state[slot]) {
        case WST_LIST_SLOT_LIVE:
            list_dump_check_live(nodes, len, slot, head, tail, state, diag);
            break;
        case WST_LIST_SLOT_FREE:
            list_dump_check_free(nodes, len, slot, state, diag);
            break;
        case WST_LIST_SLOT_ORPHAN:
        default:
            wst_list_diag_add(diag, "slot %u is neither live nor free", slot);
            break;
        }
    }
}

static void list_dump_graph(const LIST_T *list, size_t issues)
{
    if (!wst_log_accepts_images())
        return;

    const LIST_NODE_T *nodes = (const LIST_NODE_T *)list->hdr.nodes;
    if (nodes == NULL || list->hdr.cap == 0)
        return;

    uint32_t len = list_dump_len(list);
    WstListSlotState *state = calloc((size_t)len + 1, sizeof(*state));
    if (state == NULL)
        return;

    list_dump_states(list, state, len);
    WstListSlotCounts counts = wst_list_slot_counts(state + 1, len);

    WstListGraph *graph = wst_list_graph_new(
        WST_LIST_STR(LIST_T),
        "len=%u\ncap=%zu\nlive=%zu\npooled=%zu\n"
        "orphan=%zu\nfree=%u\nissues=%zu",
        len, list->hdr.cap, counts.live, counts.pooled, counts.orphan,
        list->hdr.free, issues);
    if (graph == NULL) {
        free(state);
        return;
    }

    for (uint32_t i = 1; i <= len; i++) {
        const LIST_NODE_T *node = &nodes[i];
        WstListSlotState node_state = state[i];

#ifdef LIST_FMT
        wst_list_graph_node(graph, i, node_state,
                            "#%u\nprev=%u\nnext=%u\ndata=" LIST_FMT, i,
                            node->hdr.prev, node->hdr.next, node->data);
#else
        wst_list_graph_node(graph, i, node_state,
                            "#%u\nprev=%u\nnext=%u\ndata=%p", i,
                            node->hdr.prev, node->hdr.next,
                            (const void *)&node->data);
#endif

        uint32_t links[] = { node->hdr.prev, node->hdr.next };
        WstListGraphEdge good[] = { WST_LIST_GRAPH_EDGE_PREV,
                                    WST_LIST_GRAPH_EDGE_NEXT };
        for (int k = 0; k < 2; k++) {
            uint32_t link = links[k];
            if (link == 0)
                continue;

            WstListGraphEdge kind = WST_LIST_GRAPH_EDGE_BAD;
            if (link <= len) {
                bool same = node_state == WST_LIST_SLOT_LIVE &&
                            state[link] == WST_LIST_SLOT_LIVE;
                if (k == 1 && node_state == WST_LIST_SLOT_FREE &&
                    state[link] == WST_LIST_SLOT_FREE)
                    same = true;
                if (same)
                    kind = good[k];
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    wst_list_graph_ref_slot(link), kind);
            } else {
                wst_list_graph_edge(graph, wst_list_graph_ref_slot(i),
                                    wst_list_graph_ref_null(), kind);
            }
        }
    }

    WstListGraphRef header = wst_list_graph_ref_header();
    WstListGraphRef null_ref = wst_list_graph_ref_null();
    uint32_t ends[] = { nodes[0].hdr.next, nodes[0].hdr.prev,
                        list->hdr.free };
    WstListGraphEdge good[] = { WST_LIST_GRAPH_EDGE_HEAD,
                                WST_LIST_GRAPH_EDGE_TAIL,
                                WST_LIST_GRAPH_EDGE_FREE_HEAD };
    WstListGraphEdge bad[] = { WST_LIST_GRAPH_EDGE_BAD_HEAD,
                               WST_LIST_GRAPH_EDGE_BAD_TAIL,
                               WST_LIST_GRAPH_EDGE_BAD_FREE };
    WstListSlotState want[] = { WST_LIST_SLOT_LIVE, WST_LIST_SLOT_LIVE,
                                WST_LIST_SLOT_FREE };

    for (int i = 0; i < 3; i++) {
        if (ends[i] == 0) {
            wst_list_graph_edge(graph, header, null_ref, good[i]);
        } else if (ends[i] <= len) {
            WstListGraphEdge kind = state[ends[i]] == want[i] ? good[i]
                                                              : bad[i];
            wst_list_graph_edge(graph, header,
                                wst_list_graph_ref_slot(ends[i]), kind);
        } else {
            wst_list_graph_edge(graph, header, null_ref, bad[i]);
        }
    }

    wst_list_graph_emit(graph, "linked list graph");
    wst_list_graph_free(graph);
    free(state);
}
