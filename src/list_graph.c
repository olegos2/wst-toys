/*
 * List graph semantics on top of the generic DOT builder. Backends only
 * describe slots and links, this file owns the visual language.
 */

#define LIST_IMPL

#include "toys/impl/list_graph_internal.h"
#include "toys/debug.h"
#include "toys/graph.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_ID "h"
#define NULL_ID "null"

/* DOT palette. */
#define DOT_QUOTE(value) "\"" value "\""

#define DOT_BG "#1c1b1f"
#define DOT_HEADER_FILL "#4f378b"
#define DOT_HEADER_TEXT "#eaddff"
#define DOT_HEADER_BORDER "#d0bcff"
#define DOT_NODE_BORDER "#49454f"
#define DOT_NODE_FILL "#211f26"
#define DOT_NODE_TEXT "#e6e1e5"
#define DOT_MUTED "#938f99"
#define DOT_EDGE_TEXT "#cac4d0"
#define DOT_LIVE "#d0bcff"
#define DOT_LIVE_FILL "#4a4458"
#define DOT_LIVE_TEXT "#e8def8"
#define DOT_FREE_FILL "#36343b"
#define DOT_FREE_TEXT "#cac4d0"
#define DOT_ORPHAN "#f2b8b5"
#define DOT_ORPHAN_FILL "#8c1d18"
#define DOT_ORPHAN_TEXT "#f9dedc"
#define DOT_PREV "#ccc2dc"
#define DOT_TAIL "#efb8c8"
#define DOT_FONT "fontname=\"monospace\""

/* DOT styles. */

static const char graph_attrs[] =
    "rankdir=TB, bgcolor=" DOT_QUOTE(DOT_BG) ", pad=0.25, "
    "nodesep=0.25, ranksep=0.35";

static const char node_attrs[] =
    "shape=box, style=\"rounded,filled\", " DOT_FONT
    ", fontsize=10, fontcolor=" DOT_QUOTE(DOT_NODE_TEXT)
    ", color=" DOT_QUOTE(DOT_NODE_BORDER)
    ", fillcolor=" DOT_QUOTE(DOT_NODE_FILL)
    ", penwidth=1.3, margin=\"0.13,0.06\"";

static const char edge_attrs[] =
    "color=" DOT_QUOTE(DOT_MUTED) ", arrowsize=0.75, " DOT_FONT
    ", fontsize=9, fontcolor=" DOT_QUOTE(DOT_EDGE_TEXT) ", penwidth=1.3";

static const char header_style[] =
    "shape=record, style=\"rounded,filled\", fillcolor="
    DOT_QUOTE(DOT_HEADER_FILL) ", fontcolor=" DOT_QUOTE(DOT_HEADER_TEXT)
    ", color=" DOT_QUOTE(DOT_HEADER_BORDER) ", penwidth=1.4";

static const char null_style[] =
    "shape=plaintext, fontcolor=" DOT_QUOTE(DOT_MUTED);

static const char live_style[] =
    "shape=record, fillcolor=" DOT_QUOTE(DOT_LIVE_FILL)
    ", fontcolor=" DOT_QUOTE(DOT_LIVE_TEXT) ", color=" DOT_QUOTE(DOT_LIVE);

static const char free_style[] =
    "shape=record, fillcolor=" DOT_QUOTE(DOT_FREE_FILL)
    ", fontcolor=" DOT_QUOTE(DOT_FREE_TEXT) ", color=" DOT_QUOTE(DOT_MUTED);

static const char orphan_style[] =
    "shape=record, fillcolor=" DOT_QUOTE(DOT_ORPHAN_FILL)
    ", fontcolor=" DOT_QUOTE(DOT_ORPHAN_TEXT)
    ", color=" DOT_QUOTE(DOT_ORPHAN);

static const char next_style[] =
    "color=" DOT_QUOTE(DOT_LIVE) ", penwidth=1.6";

static const char prev_style[] =
    "style=dashed, color=" DOT_QUOTE(DOT_PREV) ", constraint=false";

static const char free_next_style[] =
    "style=dashed, color=" DOT_QUOTE(DOT_MUTED);

static const char head_style[] =
    "label=\"HEAD\", color=" DOT_QUOTE(DOT_LIVE)
    ", fontcolor=" DOT_QUOTE(DOT_LIVE) ", penwidth=1.6";

static const char tail_style[] =
    "label=\"TAIL\", style=dashed, color=" DOT_QUOTE(DOT_TAIL)
    ", fontcolor=" DOT_QUOTE(DOT_TAIL);

static const char free_head_style[] =
    "label=\"FREE\", style=dotted, color=" DOT_QUOTE(DOT_MUTED)
    ", fontcolor=" DOT_QUOTE(DOT_MUTED);

static const char bad_style[] =
    "color=" DOT_QUOTE(DOT_ORPHAN) ", style=dashed, penwidth=1.5";

static const char bad_head_style[] =
    "label=\"HEAD?\", color=" DOT_QUOTE(DOT_ORPHAN)
    ", style=dashed, fontcolor=" DOT_QUOTE(DOT_ORPHAN);

static const char bad_tail_style[] =
    "label=\"TAIL?\", color=" DOT_QUOTE(DOT_ORPHAN)
    ", style=dashed, fontcolor=" DOT_QUOTE(DOT_ORPHAN);

static const char bad_free_style[] =
    "label=\"FREE?\", color=" DOT_QUOTE(DOT_ORPHAN)
    ", style=dashed, fontcolor=" DOT_QUOTE(DOT_ORPHAN);

static const char *const state_styles[] = {
    [WST_LIST_SLOT_LIVE] = live_style,
    [WST_LIST_SLOT_FREE] = free_style,
    [WST_LIST_SLOT_ORPHAN] = orphan_style,
};

static const char *const edge_styles[] = {
    [WST_LIST_GRAPH_EDGE_NEXT] = next_style,
    [WST_LIST_GRAPH_EDGE_PREV] = prev_style,
    [WST_LIST_GRAPH_EDGE_FREE_NEXT] = free_next_style,
    [WST_LIST_GRAPH_EDGE_HEAD] = head_style,
    [WST_LIST_GRAPH_EDGE_TAIL] = tail_style,
    [WST_LIST_GRAPH_EDGE_FREE_HEAD] = free_head_style,
    [WST_LIST_GRAPH_EDGE_BAD] = bad_style,
    [WST_LIST_GRAPH_EDGE_BAD_HEAD] = bad_head_style,
    [WST_LIST_GRAPH_EDGE_BAD_TAIL] = bad_tail_style,
    [WST_LIST_GRAPH_EDGE_BAD_FREE] = bad_free_style,
};

struct WstListGraph {
    WstGraph *graph;
    bool null_added;
};

/* Graph builder. */

static char *format_alloc(const char *fmt, va_list args)
{
    va_list measure;
    va_copy(measure, args);
    int len = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);
    if (len < 0)
        return NULL;

    char *text = malloc((size_t)len + 1);
    if (text == NULL)
        return NULL;

    vsnprintf(text, (size_t)len + 1, fmt, args);
    return text;
}

/* Escape field text for a raw record label. */
static void record_field(FILE *stream, const char *text, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        switch (c) {
        case '\\':
            fputs("\\\\", stream);
            break;
        case '"':
            fputs("\\\"", stream);
            break;
        case '{':
        case '}':
        case '|':
        case '<':
        case '>':
            fputc('\\', stream);
            fputc(c, stream);
            break;
        default:
            if (c < 0x20)
                fputc(' ', stream);
            else
                fputc(c, stream);
            break;
        }
    }
}

/* A tab splits one label row into side-by-side record fields. */
static void record_row(FILE *stream, const char *text, size_t len)
{
    const char *tab = memchr(text, '\t', len);
    if (tab == NULL) {
        record_field(stream, text, len);
        return;
    }

    fputs("{ ", stream);
    record_field(stream, text, (size_t)(tab - text));
    fputs(" | ", stream);
    record_field(stream, tab + 1, len - (size_t)(tab - text) - 1);
    fputs(" }", stream);
}

/* Turn newline-separated label text into stacked record rows. */
static char *record_label(const char *label)
{
    if (label == NULL)
        return NULL;

    char *record = NULL;
    size_t length = 0;
    FILE *stream = open_memstream(&record, &length);
    if (stream == NULL)
        return NULL;

    fputs("{ ", stream);
    const char *line = label;
    bool first = true;
    while (*line != '\0') {
        const char *end = strchr(line, '\n');
        size_t len = end != NULL ? (size_t)(end - line) : strlen(line);
        if (!first)
            fputs(" | ", stream);
        record_row(stream, line, len);
        first = false;
        if (end == NULL)
            break;
        line = end + 1;
    }
    fputs(" }", stream);

    if (fclose(stream) != 0) {
        free(record);
        return NULL;
    }
    return record;
}

static bool graph_record_node(WstGraph *graph, const char *id,
                              const char *label, const char *attrs)
{
    char *record = record_label(label);
    if (record == NULL)
        return false;

    bool ok = wst_graph_node_raw(graph, id, record, attrs);
    free(record);
    return ok;
}

static const char *ref_id(WstListGraphRef ref, char buf[32])
{
    switch (ref.type) {
    case WST_LIST_GRAPH_REF_HEADER:
        return HEADER_ID;
    case WST_LIST_GRAPH_REF_SLOT:
        snprintf(buf, 32, "n%zu", ref.slot);
        return buf;
    case WST_LIST_GRAPH_REF_NULL:
    default:
        return NULL_ID;
    }
}

WstListGraph *wst_list_graph_new(const char *type, const char *detail_fmt, ...)
{
    const char *label_type = type != NULL ? type : "list";

    WstListGraph *graph = calloc(1, sizeof(*graph));
    if (graph == NULL)
        return NULL;

    graph->graph = wst_graph_new(label_type);
    if (graph->graph == NULL) {
        free(graph);
        return NULL;
    }

    wst_graph_graph_attrs(graph->graph, graph_attrs);
    wst_graph_node_attrs(graph->graph, node_attrs);
    wst_graph_edge_attrs(graph->graph, edge_attrs);

    char *details = NULL;
    if (detail_fmt != NULL) {
        va_list args;
        va_start(args, detail_fmt);
        details = format_alloc(detail_fmt, args);
        va_end(args);
    }

    if (details != NULL) {
        size_t label_len = strlen(label_type) + strlen(details) + 2;
        char *label = malloc(label_len);
        if (label != NULL) {
            snprintf(label, label_len, "%s\n%s", label_type, details);
            graph_record_node(graph->graph, HEADER_ID, label, header_style);
            free(label);
        }
        free(details);
    } else {
        graph_record_node(graph->graph, HEADER_ID, label_type, header_style);
    }

    return graph;
}

void wst_list_graph_free(WstListGraph *graph)
{
    if (graph == NULL)
        return;

    wst_graph_free(graph->graph);
    free(graph);
}

void wst_list_graph_node(WstListGraph *graph, size_t slot,
                         WstListSlotState state, const char *label_fmt, ...)
{
    if (graph == NULL || label_fmt == NULL)
        return;

    char id[32];
    snprintf(id, sizeof(id), "n%zu", slot);

    va_list args;
    va_start(args, label_fmt);
    char *label = format_alloc(label_fmt, args);
    va_end(args);
    if (label == NULL)
        return;

    graph_record_node(graph->graph, id, label, state_styles[state]);
    free(label);
}

void wst_list_graph_block_begin(WstListGraph *graph, size_t block,
                                size_t cap, const void *addr)
{
    if (graph == NULL)
        return;

    char name[32];
    snprintf(name, sizeof(name), "cluster_b%zu", block);

    char attrs[192];
    snprintf(attrs, sizeof(attrs),
             "label=\"block[%zu] @%p cap=%zu\", color=\"%s\", "
             "fontcolor=\"%s\", style=\"rounded,dashed\"",
             block, addr, cap, DOT_NODE_BORDER, DOT_EDGE_TEXT);
    wst_graph_subgraph_begin(graph->graph, name, attrs);
}

void wst_list_graph_block_end(WstListGraph *graph)
{
    if (graph != NULL)
        wst_graph_subgraph_end(graph->graph);
}

void wst_list_graph_edge(WstListGraph *graph, WstListGraphRef from,
                         WstListGraphRef to, WstListGraphEdge edge)
{
    if (graph == NULL)
        return;

    if ((from.type == WST_LIST_GRAPH_REF_NULL ||
         to.type == WST_LIST_GRAPH_REF_NULL) &&
        !graph->null_added) {
        wst_graph_node(graph->graph, NULL_ID, "NULL", null_style);
        graph->null_added = true;
    }

    char from_buf[32];
    char to_buf[32];
    const char *from_id = ref_id(from, from_buf);
    const char *to_id = ref_id(to, to_buf);
    wst_graph_edge(graph->graph, from_id, to_id, edge_styles[edge]);
}

bool wst_list_graph_emit(WstListGraph *graph, const char *alt)
{
    if (graph == NULL)
        return false;

    const char *dot = wst_graph_dot(graph->graph);
    if (dot == NULL)
        return false;

    const char *svg = wst_log_graphviz(dot, "list");
    if (svg == NULL)
        return false;

    return wst_log_image(svg, alt);
}
