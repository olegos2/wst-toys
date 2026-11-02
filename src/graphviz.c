#include "toys/graphviz.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

#include <stdbool.h>
#include <stdlib.h>

struct WstGraphviz {
    GVC_t *gvc;
};

WstGraphviz *wst_graphviz_new(void)
{
    WstGraphviz *graphviz = calloc(1, sizeof(*graphviz));
    if (graphviz == NULL)
        return NULL;

    graphviz->gvc = gvContext();
    if (graphviz->gvc == NULL) {
        free(graphviz);
        return NULL;
    }
    return graphviz;
}

void wst_graphviz_free(WstGraphviz *graphviz)
{
    if (graphviz == NULL)
        return;

    if (graphviz->gvc != NULL)
        gvFreeContext(graphviz->gvc);
    free(graphviz);
}

bool wst_graphviz_render(WstGraphviz *graphviz, const char *dot,
                         const char *format, const char *path)
{
    if (graphviz == NULL || graphviz->gvc == NULL || dot == NULL ||
        format == NULL || path == NULL)
        return false;

    Agraph_t *graph = agmemread(dot);
    if (graph == NULL)
        return false;

    bool ok = false;
    if (gvLayout(graphviz->gvc, graph, "dot") == 0) {
        ok = gvRenderFilename(graphviz->gvc, graph, format, path) == 0;
        gvFreeLayout(graphviz->gvc, graph);
    }

    agclose(graph);
    return ok;
}
