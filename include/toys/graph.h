#ifndef TOYS_GRAPH_H
#define TOYS_GRAPH_H

#include <stdbool.h>

/** Small Graphviz dot builder. */
typedef struct WstGraph WstGraph;

/** Start a directed graph named `name`, or `"graph"` when NULL. */
WstGraph *wst_graph_new(const char *name);

/** Free the builder and any rendered source. */
void wst_graph_free(WstGraph *graph);

/** Add default graph attributes, for example `rankdir=LR`. */
bool wst_graph_graph_attrs(WstGraph *graph, const char *attrs);

/** Add default node attributes. */
bool wst_graph_node_attrs(WstGraph *graph, const char *attrs);

/** Add default edge attributes. */
bool wst_graph_edge_attrs(WstGraph *graph, const char *attrs);

/**
 * Add a node.
 *
 * `id` names the node, `label` is shown in the graph, and `attrs` appends
 * per-node attributes. Labels may contain newlines.
 */
bool wst_graph_node(WstGraph *graph, const char *id, const char *label,
                    const char *attrs);

/**
 * Add a node with a raw DOT label.
 *
 * The label is emitted verbatim. Callers must escape quotes, backslashes,
 * and any DOT or record label syntax themselves.
 */
bool wst_graph_node_raw(WstGraph *graph, const char *id, const char *label,
                        const char *attrs);

/** Add a node with a printf-formatted label. */
bool wst_graph_nodef(WstGraph *graph, const char *id, const char *attrs,
                     const char *label_fmt, ...);

/** Add a directed edge. */
bool wst_graph_edge(WstGraph *graph, const char *from, const char *to,
                    const char *attrs);

/**
 * Start a named DOT subgraph.
 *
 * Prefix the name with `cluster_` to get a visual cluster. `attrs` is
 * emitted as the subgraph attribute statement.
 */
bool wst_graph_subgraph_begin(WstGraph *graph, const char *name,
                              const char *attrs);

/** End the current subgraph. */
bool wst_graph_subgraph_end(WstGraph *graph);

/**
 * Finalize and return the DOT source.
 *
 * The pointer is owned by `graph` and stays valid until `wst_graph_free`.
 * Returns NULL when the builder is invalid or writing failed. No nodes or
 * edges may be added after this call.
 */
const char *wst_graph_dot(WstGraph *graph);

#endif /* TOYS_GRAPH_H */
