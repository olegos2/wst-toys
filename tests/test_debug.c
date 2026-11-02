#include "tests_common.h"
#include "log_util.h"

#include "toys/debug.h"
#include "toys/graph.h"
#include "toys/graphviz.h"

#include <stdlib.h>
#include <string.h>

static void test_text_log(void)
{
    char *dir = test_tmpdir("wst-text");
    CHECK(dir != NULL, "temp dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_TEXT), "open text log");
    CHECK(wst_log_format() == WST_LOG_FORMAT_TEXT, "text log format");
    CHECK(!wst_log_accepts_images(), "text log rejects images");

    const char *live_path = wst_log_path();
    CHECK(live_path != NULL, "text log path");
    char *path = live_path != NULL ? strdup(live_path) : NULL;
    CHECK(path != NULL && test_endswith(path, ".log"), "text extension");

    wst_log_print_at(WST_LOG_WARN, "file.c", "func", 42, "value=%d\n", 7);
    wst_log_close();
    wst_log_close();
    CHECK(wst_log_path() == NULL, "path cleared after close");

    char *data = path != NULL ? test_read_file(path) : NULL;
    CHECK(data != NULL, "read text log");
    if (data != NULL) {
        CHECK(strstr(data, "[W] file.c:func: 42: value=7") != NULL,
              "text line format");
        CHECK(strstr(data, "<html>") == NULL, "text log is not html");
    }

    free(data);
    free(path);
    test_rm_rf(dir);
    free(dir);
}

static void test_unique_names(void)
{
    char *dir = test_tmpdir("wst-unique");
    CHECK(dir != NULL, "temp dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_TEXT), "first open");
    char *first = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
    wst_log_close();

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_TEXT), "second open");
    char *second = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
    wst_log_close();

    CHECK(first != NULL && second != NULL, "both paths");
    CHECK(first != NULL && second != NULL && strcmp(first, second) != 0,
          "collision gets a new name");

    char *first_data = first != NULL ? test_read_file(first) : NULL;
    char *second_data = second != NULL ? test_read_file(second) : NULL;
    CHECK(first_data != NULL, "first file exists");
    CHECK(second_data != NULL, "second file exists");

    free(first_data);
    free(second_data);
    free(first);
    free(second);
    test_rm_rf(dir);
    free(dir);
}

static void test_html_log(void)
{
    char *dir = test_tmpdir("wst-html");
    CHECK(dir != NULL, "temp dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open html log");
    CHECK(wst_log_format() == WST_LOG_FORMAT_HTML, "html format");
    CHECK(wst_log_accepts_images(), "html accepts images");

    char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
    CHECK(path != NULL && test_endswith(path, ".html"), "html extension");

    wst_log_print_at(WST_LOG_INFO, "x.c", "fn", 7,
                     "<tag> & \"q\" 'a'\n");
    wst_log_print(WST_LOG_DEBUG, "plain\n");
    wst_log_close();

    char *data = path != NULL ? test_read_file(path) : NULL;
    CHECK(data != NULL, "read html log");
    if (data != NULL) {
        CHECK(strncmp(data, "<!doctype html>", 15) == 0, "doctype");
        CHECK(strstr(data, "<pre class=\"log\">") != NULL, "log pre block");
        CHECK(strstr(data, "class=\"prio prio-i\"") != NULL, "info class");
        CHECK(strstr(data, "class=\"loc-file\">x.c") != NULL, "file span");
        CHECK(strstr(data, "class=\"loc-func\">fn") != NULL, "func span");
        CHECK(strstr(data, "class=\"loc-line\">  7<") != NULL, "line span");
        CHECK(strstr(data, "&lt;tag&gt; &amp; &quot;q&quot; &#39;a&#39;") !=
                  NULL, "html escaping");
        CHECK(strstr(data, "<tag>") == NULL, "no raw tags");
        CHECK(strstr(data, "--surface: #1c1b1f") != NULL,
              "dark surface palette");
        CHECK(strstr(data, "--primary: #d0bcff") != NULL,
              "light primary palette");
        CHECK(strstr(data, "border-radius: var(--shape-sm)") != NULL,
              "chip shape");
        CHECK(strstr(data, "</pre>\n</main>\n</body>\n</html>") != NULL,
              "html footer");
    }

    free(data);
    free(path);
    test_rm_rf(dir);
    free(dir);
}

static void test_html_filter(void)
{
    char *dir = test_tmpdir("wst-filter");
    CHECK(dir != NULL, "temp dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open filtered log");
    char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;

    wst_log_set_max_prio(WST_LOG_ERROR);
    wst_log_print(WST_LOG_INFO, "hidden line\n");
    wst_log_print(WST_LOG_ERROR, "shown line\n");
    wst_log_close();
    wst_log_set_max_prio(WST_LOG_VERBOSE);

    char *data = path != NULL ? test_read_file(path) : NULL;
    CHECK(data != NULL, "read filtered log");
    if (data != NULL) {
        CHECK(strstr(data, "hidden line") == NULL, "priority filter hides");
        CHECK(strstr(data, "shown line") != NULL, "priority filter shows");
    }

    free(data);
    free(path);
    test_rm_rf(dir);
    free(dir);
}

static void test_stderr_fallback(void)
{
    CHECK(!wst_log_open(NULL, WST_LOG_FORMAT_HTML),
          "html without directory fails");
    CHECK(wst_log_format() == WST_LOG_FORMAT_TEXT, "fallback is text");
    CHECK(wst_log_path() == NULL, "fallback has no path");
    CHECK(!wst_log_accepts_images(), "fallback rejects images");

    CHECK(wst_log_open(NULL, WST_LOG_FORMAT_TEXT), "text on stderr");
    CHECK(wst_log_path() == NULL, "stderr has no path");
    wst_log_close();
}

static void test_images(void)
{
    char *text_dir = test_tmpdir("wst-image-text");
    CHECK(text_dir != NULL, "text image dir");
    if (text_dir != NULL) {
        CHECK(wst_log_open(text_dir, WST_LOG_FORMAT_TEXT), "open text image log");
        char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
        CHECK(wst_log_image("pic.png", "alt"), "text image");
        wst_log_close();

        char *data = path != NULL ? test_read_file(path) : NULL;
        CHECK(data != NULL && strstr(data, "[image] pic.png - alt") != NULL,
              "text image line");

        free(data);
        free(path);
        test_rm_rf(text_dir);
        free(text_dir);
    }

    char *html_dir = test_tmpdir("wst-image-html");
    CHECK(html_dir != NULL, "html image dir");
    if (html_dir != NULL) {
        CHECK(wst_log_open(html_dir, WST_LOG_FORMAT_HTML), "open html image log");
        char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
        CHECK(wst_log_image("a&b.png", "x<y"), "html image");
        wst_log_close();

        char *data = path != NULL ? test_read_file(path) : NULL;
        CHECK(data != NULL &&
                  strstr(data, "<img class=\"log-image\" src=\"a&amp;b.png\" "
                               "alt=\"x&lt;y\">") != NULL,
              "html image tag");

        free(data);
        free(path);
        test_rm_rf(html_dir);
        free(html_dir);
    }
}

static void test_graph_builder(void)
{
    WstGraph *graph = wst_graph_new("bad\"name");
    CHECK(graph != NULL, "new graph");
    if (graph == NULL)
        return;

    CHECK(wst_graph_graph_attrs(graph, "rankdir=LR"), "graph attrs");
    CHECK(wst_graph_node_attrs(graph, "shape=box"), "node attrs");
    CHECK(wst_graph_edge_attrs(graph, "color=\"#123456\""), "edge attrs");
    CHECK(wst_graph_nodef(graph, "a", NULL, "line=%d\nnext", 7),
          "formatted node");
    CHECK(wst_graph_node(graph, "b", "B\"quote", NULL), "plain node");
    CHECK(wst_graph_subgraph_begin(graph, "cluster_test", "label=\"test\""),
          "subgraph begin");
    CHECK(wst_graph_node_raw(graph, "raw", "{ raw | value }", NULL),
          "raw node");
    CHECK(wst_graph_subgraph_end(graph), "subgraph end");
    CHECK(wst_graph_edge(graph, "a", "b", "style=dashed"), "edge");

    const char *dot = wst_graph_dot(graph);
    CHECK(dot != NULL, "render dot");
    if (dot != NULL) {
        CHECK(strstr(dot, "digraph \"bad\\\"name\" {") != NULL, "graph name");
        CHECK(strstr(dot, "graph [rankdir=LR];") != NULL, "graph attrs out");
        CHECK(strstr(dot, "\"a\" [label=\"line=7\\nnext\"];") != NULL,
              "formatted label");
        CHECK(strstr(dot, "B\\\"quote") != NULL, "label quote escape");
        CHECK(strstr(dot, "subgraph \"cluster_test\" {") != NULL,
              "subgraph out");
        CHECK(strstr(dot, "    graph [label=\"test\"];") != NULL,
              "subgraph attrs out");
        CHECK(strstr(dot, "\"raw\" [label=\"{ raw | value }\"];") != NULL,
              "raw label out");
        CHECK(strstr(dot, "\"a\" -> \"b\" [style=dashed];") != NULL,
              "edge out");
    }

    CHECK(!wst_graph_node(graph, "c", "C", NULL), "finalized graph is closed");
    CHECK(!wst_graph_edge(graph, "b", "c", NULL), "finalized edge is closed");
    wst_graph_free(graph);
    wst_graph_free(NULL);
}

static void test_graphviz_module(void)
{
    char *dir = test_tmpdir("wst-gvc");
    CHECK(dir != NULL, "graphviz dir");
    if (dir == NULL)
        return;

    char *svg = test_path_join(dir, "direct.svg");
    WstGraphviz *graphviz = wst_graphviz_new();
    CHECK(graphviz != NULL, "graphviz context");
    CHECK(svg != NULL && graphviz != NULL &&
              wst_graphviz_render(graphviz,
                                  "digraph { a -> b }", "svg", svg),
          "direct render");
    wst_graphviz_free(graphviz);

    char *data = svg != NULL ? test_read_file(svg) : NULL;
    CHECK(data != NULL && strstr(data, "<svg") != NULL, "direct svg");

    free(data);
    free(svg);
    test_rm_rf(dir);
    free(dir);
}

static void test_graphviz_log(void)
{
    char *dir = test_tmpdir("wst-gvc-log");
    CHECK(dir != NULL, "graphviz log dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open graph log");
    char *log_path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;

    const char *svg = wst_log_graphviz("digraph { a -> b }", "demo");
    CHECK(svg != NULL && strcmp(svg, "demo.svg") == 0, "graphviz log path");
    CHECK(svg != NULL && wst_log_image(svg, "demo graph"), "insert graph image");

    const char *svg2 = wst_log_graphviz("digraph { a -> c }", "demo");
    CHECK(svg2 != NULL && strcmp(svg2, "demo-1.svg") == 0,
          "graph name collision");
    CHECK(svg2 != NULL && wst_log_image(svg2, "second graph"),
          "insert second graph image");

    char *dot_path = test_path_join(dir, "demo.dot");
    char *svg_path = test_path_join(dir, "demo.svg");
    char *dot2_path = test_path_join(dir, "demo-1.dot");
    char *svg2_path = test_path_join(dir, "demo-1.svg");
    char *dot = dot_path != NULL ? test_read_file(dot_path) : NULL;
    char *svg_data = svg_path != NULL ? test_read_file(svg_path) : NULL;
    char *dot2 = dot2_path != NULL ? test_read_file(dot2_path) : NULL;
    char *svg2_data = svg2_path != NULL ? test_read_file(svg2_path) : NULL;
    CHECK(dot != NULL && strstr(dot, "digraph") != NULL, "dot file");
    CHECK(svg_data != NULL && strstr(svg_data, "<svg") != NULL, "svg file");
    CHECK(dot2 != NULL && strstr(dot2, "a -> c") != NULL, "second dot file");
    CHECK(svg2_data != NULL && strstr(svg2_data, "<svg") != NULL,
          "second svg file");

    wst_log_close();

    char *data = log_path != NULL ? test_read_file(log_path) : NULL;
    CHECK(data != NULL &&
              strstr(data,
                     "<img class=\"log-image\" src=\"demo.svg\" "
                     "alt=\"demo graph\">") != NULL,
          "graph image tag");
    CHECK(data != NULL &&
              strstr(data,
                     "<img class=\"log-image\" src=\"demo-1.svg\" "
                     "alt=\"second graph\">") != NULL,
          "second graph image tag");

    free(dot);
    free(svg_data);
    free(dot2);
    free(svg2_data);
    free(dot_path);
    free(svg_path);
    free(dot2_path);
    free(svg2_path);
    free(data);
    free(log_path);
    test_rm_rf(dir);
    free(dir);
}

static void test_graphviz_failure(void)
{
    char *dir = test_tmpdir("wst-gvc-bad");
    CHECK(dir != NULL, "graphviz bad dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open bad graph log");
    char *log_path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;

    CHECK(wst_log_graphviz("digraph {", "bad") == NULL, "bad dot rejected");
    wst_log_close();

    char *data = log_path != NULL ? test_read_file(log_path) : NULL;
    CHECK(data != NULL && strstr(data, "graphviz render failed") != NULL,
          "graphviz failure warning");

    free(data);
    free(log_path);
    test_rm_rf(dir);
    free(dir);
}

int main(void)
{
    wst_log_set_max_prio(WST_LOG_VERBOSE);

    test_text_log();
    test_unique_names();
    test_html_log();
    test_html_filter();
    test_stderr_fallback();
    test_images();
    test_graph_builder();
    test_graphviz_module();
    test_graphviz_log();
    test_graphviz_failure();

    wst_log_close();
    return tests_summary();
}
