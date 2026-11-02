#ifndef TOYS_GRAPHVIZ_H
#define TOYS_GRAPHVIZ_H

#include <stdbool.h>

/**
 * Reusable Graphviz renderer.
 *
 * One context can render any number of DOT sources.
 */
typedef struct WstGraphviz WstGraphviz;

/** Create a rendering context, or NULL on failure. */
WstGraphviz *wst_graphviz_new(void);

/** Free a rendering context. */
void wst_graphviz_free(WstGraphviz *graphviz);

/**
 * Render DOT source to `path` using `format`, for example `"svg"`.
 *
 * Returns false on parse, layout, or render failure.
 */
bool wst_graphviz_render(WstGraphviz *graphviz, const char *dot,
                         const char *format, const char *path);

#endif /* TOYS_GRAPHVIZ_H */
