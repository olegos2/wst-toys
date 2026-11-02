#include "debug_internal.h"
#include "toys/graphviz.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Kept outside WstLogState so the core logger does not depend on Graphviz.
 * Only programs that render graphs pull this module and its library in.
 */
static WstGraphviz *graphviz_context;
static bool graphviz_atexit_registered;

static void graphviz_cleanup(void)
{
    wst_graphviz_free(graphviz_context);
    graphviz_context = NULL;
}

static bool graph_paths_free(const char *dir, const char *stem,
                             char **dot_path, char **svg_path)
{
    free(*dot_path);
    free(*svg_path);
    *dot_path = wst_log_path_join(dir, stem, ".dot");
    *svg_path = wst_log_path_join(dir, stem, ".svg");
    return *dot_path != NULL && *svg_path != NULL &&
           access(*dot_path, F_OK) != 0 && access(*svg_path, F_OK) != 0;
}

const char *wst_log_graphviz(const char *dot_source, const char *name)
{
    WstLogState *state = wst_log_state();
    if (state->dir == NULL || dot_source == NULL)
        return NULL;

    char *stem = wst_log_sanitize_name(name);
    if (stem == NULL)
        return NULL;

    char *dot_path = NULL;
    char *svg_path = NULL;
    char *unique = NULL;
    bool reserved = false;

    for (unsigned n = 0; n < 10000; n++) {
        char suffix[16] = "";
        if (n > 0)
            snprintf(suffix, sizeof(suffix), "-%u", n);

        free(unique);
        size_t len = strlen(stem) + strlen(suffix) + 1;
        unique = malloc(len);
        if (unique == NULL)
            goto fail;
        snprintf(unique, len, "%s%s", stem, suffix);

        if (graph_paths_free(state->dir, unique, &dot_path, &svg_path)) {
            reserved = true;
            break;
        }
    }

    if (!reserved)
        goto fail;

    FILE *dot = fopen(dot_path, "w");
    if (dot == NULL)
        goto fail;
    bool write_ok = fputs(dot_source, dot) >= 0;
    if (fclose(dot) != 0)
        write_ok = false;
    if (!write_ok)
        goto fail;

    if (graphviz_context == NULL)
        graphviz_context = wst_graphviz_new();
    if (!graphviz_atexit_registered) {
        atexit(graphviz_cleanup);
        graphviz_atexit_registered = true;
    }
    if (graphviz_context == NULL ||
        !wst_graphviz_render(graphviz_context, dot_source, "svg", svg_path) ||
        access(svg_path, F_OK) != 0) {
        LOG_W("graphviz render failed for %s", svg_path);
        goto fail;
    }

    free(state->graph_path);
    state->graph_path = wst_log_path_join("", unique, ".svg");
    if (state->graph_path == NULL)
        goto fail;

    free(stem);
    free(dot_path);
    free(svg_path);
    free(unique);
    return state->graph_path;

fail:
    free(stem);
    free(dot_path);
    free(svg_path);
    free(unique);
    return NULL;
}

bool wst_log_image(const char *path, const char *alt)
{
    if (path == NULL)
        return false;

    WstLogState *state = wst_log_state();
    FILE *stream = wst_log_stream();

    if (state->format == WST_LOG_FORMAT_HTML) {
        fputs("<span class=\"image-wrap\"><img class=\"log-image\" src=\"",
              stream);
        wst_log_html_escape(stream, path);
        fputs("\" alt=\"", stream);
        wst_log_html_escape(stream, alt != NULL ? alt : "");
        fputs("\"></span>\n", stream);
    } else {
        fprintf(stream, "[image] %s", path);
        if (alt != NULL)
            fprintf(stream, " - %s", alt);
        fputc('\n', stream);
    }

    wst_log_flush();
    return true;
}
