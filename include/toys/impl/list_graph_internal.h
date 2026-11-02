#ifndef TOYS_LIST_GRAPH_INTERNAL_H
#define TOYS_LIST_GRAPH_INTERNAL_H

#include "list_common_internal.h"

#include <stdbool.h>
#include <stddef.h>

/** List edge types, each with its own style. */
typedef enum {
    WST_LIST_GRAPH_EDGE_NEXT,
    WST_LIST_GRAPH_EDGE_PREV,
    WST_LIST_GRAPH_EDGE_FREE_NEXT,
    WST_LIST_GRAPH_EDGE_HEAD,
    WST_LIST_GRAPH_EDGE_TAIL,
    WST_LIST_GRAPH_EDGE_FREE_HEAD,
    WST_LIST_GRAPH_EDGE_BAD,
    WST_LIST_GRAPH_EDGE_BAD_HEAD,
    WST_LIST_GRAPH_EDGE_BAD_TAIL,
    WST_LIST_GRAPH_EDGE_BAD_FREE,
} WstListGraphEdge;

/** Node reference used by graph edges. */
typedef enum {
    WST_LIST_GRAPH_REF_HEADER,
    WST_LIST_GRAPH_REF_SLOT,
    WST_LIST_GRAPH_REF_NULL,
} WstListGraphRefType;

typedef struct {
    WstListGraphRefType type;
    size_t slot;
} WstListGraphRef;

typedef struct WstListGraph WstListGraph;

static inline WstListGraphRef wst_list_graph_ref_header(void)
{
    return (WstListGraphRef){ .type = WST_LIST_GRAPH_REF_HEADER, .slot = 0 };
}

static inline WstListGraphRef wst_list_graph_ref_slot(size_t slot)
{
    return (WstListGraphRef){ .type = WST_LIST_GRAPH_REF_SLOT, .slot = slot };
}

static inline WstListGraphRef wst_list_graph_ref_null(void)
{
    return (WstListGraphRef){ .type = WST_LIST_GRAPH_REF_NULL, .slot = 0 };
}

/** Start a themed list graph with a header node. */
WstListGraph *wst_list_graph_new(const char *type, const char *detail_fmt, ...);

/** Free the graph and its DOT source. */
void wst_list_graph_free(WstListGraph *graph);

/** Add a slot node with a formatted label. */
void wst_list_graph_node(WstListGraph *graph, size_t slot,
                         WstListSlotState state, const char *label_fmt, ...);

/** Start a cluster for one pointer-backend node block. */
void wst_list_graph_block_begin(WstListGraph *graph, size_t block,
                                size_t cap, const void *addr);

/** End the current block cluster. */
void wst_list_graph_block_end(WstListGraph *graph);

/** Add a styled edge between two node references. */
void wst_list_graph_edge(WstListGraph *graph, WstListGraphRef from,
                         WstListGraphRef to, WstListGraphEdge edge);

/** Render the graph and insert it into the current log. */
bool wst_list_graph_emit(WstListGraph *graph, const char *alt);

#endif /* TOYS_LIST_GRAPH_INTERNAL_H */
