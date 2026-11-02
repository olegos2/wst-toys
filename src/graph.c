#include "toys/graph.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct WstGraph {
    FILE *stream;
    char *buf;
    size_t len;
    bool broken;
    bool finalized;
};

static void dot_escape(FILE *stream, const char *text)
{
    if (text == NULL)
        return;

    for (const unsigned char *p = (const unsigned char *)text; *p != '\0';
         p++) {
        switch (*p) {
        case '\\':
            fputs("\\\\", stream);
            break;
        case '"':
            fputs("\\\"", stream);
            break;
        case '\n':
            fputs("\\n", stream);
            break;
        case '\r':
            fputs("\\r", stream);
            break;
        case '\t':
            fputs("\\t", stream);
            break;
        default:
            if (*p < 0x20)
                fputc(' ', stream);
            else
                fputc(*p, stream);
            break;
        }
    }
}

static bool graph_ready(const WstGraph *graph)
{
    return graph != NULL && graph->stream != NULL && !graph->broken;
}

WstGraph *wst_graph_new(const char *name)
{
    WstGraph *graph = calloc(1, sizeof(*graph));
    if (graph == NULL)
        return NULL;

    graph->stream = open_memstream(&graph->buf, &graph->len);
    if (graph->stream == NULL) {
        free(graph);
        return NULL;
    }

    fputs("digraph \"", graph->stream);
    dot_escape(graph->stream, name != NULL ? name : "graph");
    fputs("\" {\n", graph->stream);

    if (ferror(graph->stream)) {
        fclose(graph->stream);
        free(graph->buf);
        free(graph);
        return NULL;
    }
    return graph;
}

void wst_graph_free(WstGraph *graph)
{
    if (graph == NULL)
        return;

    if (graph->stream != NULL)
        fclose(graph->stream);
    free(graph->buf);
    free(graph);
}

static bool graph_default_attrs(WstGraph *graph, const char *kind,
                                const char *attrs)
{
    if (!graph_ready(graph))
        return false;
    if (attrs == NULL || attrs[0] == '\0')
        return true;

    fprintf(graph->stream, "  %s [%s];\n", kind, attrs);
    if (ferror(graph->stream)) {
        graph->broken = true;
        return false;
    }
    return true;
}

bool wst_graph_graph_attrs(WstGraph *graph, const char *attrs)
{
    return graph_default_attrs(graph, "graph", attrs);
}

bool wst_graph_node_attrs(WstGraph *graph, const char *attrs)
{
    return graph_default_attrs(graph, "node", attrs);
}

bool wst_graph_edge_attrs(WstGraph *graph, const char *attrs)
{
    return graph_default_attrs(graph, "edge", attrs);
}

static bool graph_node(WstGraph *graph, const char *id, const char *label,
                       const char *attrs, bool raw)
{
    if (!graph_ready(graph) || id == NULL)
        return false;

    fputs("  \"", graph->stream);
    dot_escape(graph->stream, id);
    fputs("\" [label=\"", graph->stream);
    if (raw)
        fputs(label != NULL ? label : "", graph->stream);
    else
        dot_escape(graph->stream, label != NULL ? label : "");
    fputc('"', graph->stream);

    if (attrs != NULL && attrs[0] != '\0')
        fprintf(graph->stream, ", %s", attrs);
    fputs("];\n", graph->stream);

    if (ferror(graph->stream)) {
        graph->broken = true;
        return false;
    }
    return true;
}

bool wst_graph_node(WstGraph *graph, const char *id, const char *label,
                    const char *attrs)
{
    return graph_node(graph, id, label, attrs, false);
}

bool wst_graph_node_raw(WstGraph *graph, const char *id, const char *label,
                        const char *attrs)
{
    return graph_node(graph, id, label, attrs, true);
}

bool wst_graph_nodef(WstGraph *graph, const char *id, const char *attrs,
                     const char *label_fmt, ...)
{
    if (label_fmt == NULL)
        return wst_graph_node(graph, id, NULL, attrs);

    va_list args;
    va_start(args, label_fmt);
    va_list measure;
    va_copy(measure, args);
    int label_len = vsnprintf(NULL, 0, label_fmt, measure);
    va_end(measure);

    if (label_len < 0) {
        va_end(args);
        return false;
    }

    char *label = malloc((size_t)label_len + 1);
    if (label == NULL) {
        va_end(args);
        return false;
    }

    vsnprintf(label, (size_t)label_len + 1, label_fmt, args);
    va_end(args);

    bool ok = wst_graph_node(graph, id, label, attrs);
    free(label);
    return ok;
}

bool wst_graph_edge(WstGraph *graph, const char *from, const char *to,
                    const char *attrs)
{
    if (!graph_ready(graph) || from == NULL || to == NULL)
        return false;

    fputs("  \"", graph->stream);
    dot_escape(graph->stream, from);
    fputs("\" -> \"", graph->stream);
    dot_escape(graph->stream, to);
    fputc('"', graph->stream);

    if (attrs != NULL && attrs[0] != '\0')
        fprintf(graph->stream, " [%s]", attrs);
    fputs(";\n", graph->stream);

    if (ferror(graph->stream)) {
        graph->broken = true;
        return false;
    }
    return true;
}

bool wst_graph_subgraph_begin(WstGraph *graph, const char *name,
                              const char *attrs)
{
    if (!graph_ready(graph) || name == NULL)
        return false;

    fputs("  subgraph \"", graph->stream);
    dot_escape(graph->stream, name);
    fputs("\" {\n", graph->stream);
    if (attrs != NULL && attrs[0] != '\0')
        fprintf(graph->stream, "    graph [%s];\n", attrs);

    if (ferror(graph->stream)) {
        graph->broken = true;
        return false;
    }
    return true;
}

bool wst_graph_subgraph_end(WstGraph *graph)
{
    if (!graph_ready(graph))
        return false;

    fputs("  }\n", graph->stream);
    if (ferror(graph->stream)) {
        graph->broken = true;
        return false;
    }
    return true;
}

const char *wst_graph_dot(WstGraph *graph)
{
    if (graph == NULL)
        return NULL;

    if (!graph->finalized) {
        if (graph->stream != NULL) {
            fputs("}\n", graph->stream);
            if (fclose(graph->stream) != 0)
                graph->broken = true;
            graph->stream = NULL;
        }
        graph->finalized = true;
    }

    return graph->broken ? NULL : graph->buf;
}
