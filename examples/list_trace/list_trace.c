#include "toys/debug.h"
#include "toys/list.h"

#include <stdio.h>
#include <stdlib.h>

/* Stamp the index-linked backend for this example. */
#define LIST_ELEM int
#define LIST_T WstListIdxInt
#define LIST_F(name) wst_list_idx_int_ ## name
#define LIST_FMT "%d"
#define LIST_IMPL
#include "toys/impl/list_idx_internal.h"

static void fail(const char *what)
{
    LOG_E("%s", what);
    wst_log_close();
    exit(EXIT_FAILURE);
}

static void require(WstListErr err, const char *what)
{
    if (err != WST_LIST_NO_ERR) {
        LOG_E("%s: %s", what, wst_list_err_str(err));
        wst_log_close();
        exit(EXIT_FAILURE);
    }
}

static void pointer_demo(void)
{
    WstListInt list = { 0 };

    LOG_I("pointer backend: append 10..100, creating two blocks");
    require(wst_list_int_init(&list), "pointer init");
    for (int value = 10; value <= 100; value += 10)
        require(wst_list_int_append(&list, value), "pointer append");
    wst_list_int_dump(&list);

    LOG_I("pointer backend: insert 55 at index 4");
    require(wst_list_int_insert(&list, 4, 55), "pointer insert");
    wst_list_int_dump(&list);

    LOG_I("pointer backend: remove index 2");
    if (!wst_list_int_remove(&list, 2))
        fail("pointer remove");
    wst_list_int_dump(&list);

    LOG_I("pointer backend: append 110, reusing the pooled node");
    require(wst_list_int_append(&list, 110), "pointer append");
    wst_list_int_dump(&list);

    require(wst_list_int_free(&list), "pointer free");
}

static void index_demo(void)
{
    WstListIdxInt list = { 0 };

    LOG_I("index backend: append 10..100");
    require(wst_list_idx_int_init(&list), "index init");
    for (int value = 10; value <= 100; value += 10)
        require(wst_list_idx_int_append(&list, value), "index append");
    wst_list_idx_int_dump(&list);

    LOG_I("index backend: insert 55 at index 4");
    require(wst_list_idx_int_insert(&list, 4, 55), "index insert");
    wst_list_idx_int_dump(&list);

    LOG_I("index backend: remove index 2");
    if (!wst_list_idx_int_remove(&list, 2))
        fail("index remove");
    wst_list_idx_int_dump(&list);

    LOG_I("index backend: append 110, reusing the free slot");
    require(wst_list_idx_int_append(&list, 110), "index append");
    wst_list_idx_int_dump(&list);

    require(wst_list_idx_int_free(&list), "index free");
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "logs";

    if (!wst_log_open(dir, WST_LOG_FORMAT_HTML)) {
        fprintf(stderr, "cannot open HTML log in '%s'\n", dir);
        return EXIT_FAILURE;
    }

    wst_log_set_max_prio(WST_LOG_INFO);
    printf("HTML log: %s\n", wst_log_path());

    LOG_I("list trace: each dump below embeds a fresh graph");
    pointer_demo();
    index_demo();
    LOG_I("list trace: done");

    wst_log_close();
    return EXIT_SUCCESS;
}
