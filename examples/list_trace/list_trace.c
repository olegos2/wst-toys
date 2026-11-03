#include "toys/argparse.h"
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

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

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

    LOG_I("append 10..100, creating two blocks");
    require(wst_list_int_init(&list), "pointer init");
    for (int value = 10; value <= 100; value += 10)
        require(wst_list_int_append(&list, value), "pointer append");
    wst_list_int_dump(&list);

    LOG_I("insert 55 at index 4");
    require(wst_list_int_insert(&list, 4, 55), "pointer insert");
    wst_list_int_dump(&list);

    LOG_I("set index 2 to 25");
    if (!wst_list_int_set(&list, 2, 25))
        fail("pointer set");
    wst_list_int_dump(&list);

    LOG_I("remove index 2");
    if (!wst_list_int_remove(&list, 2))
        fail("pointer remove");
    wst_list_int_dump(&list);

    LOG_I("append 110, reusing the pooled node");
    require(wst_list_int_append(&list, 110), "pointer append");
    wst_list_int_dump(&list);

    require(wst_list_int_free(&list), "pointer free");
}

static void pointer_corruption(void)
{
    WstListInt list = { 0 };

    require(wst_list_int_init(&list), "corrupt init");
    for (int value = 10; value <= 50; value += 10)
        require(wst_list_int_append(&list, value), "corrupt append");
    if (!wst_list_int_remove(&list, 1))
        fail("corrupt remove");

    WstListIntNode *first = wst_list_int_get_node(&list, 0);
    WstListIntNode *second = wst_list_int_get_node(&list, 1);
    WstListIntNode *third = wst_list_int_get_node(&list, 2);
    WstListIntNode *last = wst_list_int_get_node(&list, 3);
    if (first == NULL || second == NULL || third == NULL || last == NULL)
        fail("corrupt nodes");

    LOG_I("corruption: live prev link points nowhere");
    WstListNodeHdr *saved_prev = second->hdr.prev;
    second->hdr.prev = NULL;
    wst_list_int_dump(&list);
    second->hdr.prev = saved_prev;

    LOG_I("corruption: live node dropped from both chains");
    WstListNodeHdr *saved_next = first->hdr.next;
    WstListNodeHdr *saved_third_prev = third->hdr.prev;
    first->hdr.next = &third->hdr;
    third->hdr.prev = &first->hdr;
    wst_list_int_dump(&list);
    first->hdr.next = saved_next;
    third->hdr.prev = saved_third_prev;

    LOG_I("corruption: free head points at a live node");
    WstListNodeHdr *saved_free = list.hdr.free;
    list.hdr.free = &first->hdr;
    wst_list_int_dump(&list);
    list.hdr.free = saved_free;

    LOG_I("corruption: tail links back to head");
    WstListNodeHdr *saved_tail_next = last->hdr.next;
    last->hdr.next = &first->hdr;
    wst_list_int_dump(&list);
    last->hdr.next = saved_tail_next;

    require(wst_list_int_free(&list), "corrupt free");
}

static void index_demo(void)
{
    WstListIdxInt list = { 0 };

    LOG_I("append 10..100");
    require(wst_list_idx_int_init(&list), "index init");
    for (int value = 10; value <= 100; value += 10)
        require(wst_list_idx_int_append(&list, value), "index append");
    wst_list_idx_int_dump(&list);

    LOG_I("insert 55 at index 4");
    require(wst_list_idx_int_insert(&list, 4, 55), "index insert");
    wst_list_idx_int_dump(&list);

    LOG_I("set index 2 to 25");
    if (!wst_list_idx_int_set(&list, 2, 25))
        fail("index set");
    wst_list_idx_int_dump(&list);

    LOG_I("remove index 2");
    if (!wst_list_idx_int_remove(&list, 2))
        fail("index remove");
    wst_list_idx_int_dump(&list);

    LOG_I("append 110, reusing the free slot");
    require(wst_list_idx_int_append(&list, 110), "index append");
    wst_list_idx_int_dump(&list);

    require(wst_list_idx_int_free(&list), "index free");
}

static void index_corruption(void)
{
    WstListIdxInt list = { 0 };

    require(wst_list_idx_int_init(&list), "corrupt init");
    for (int value = 10; value <= 50; value += 10)
        require(wst_list_idx_int_append(&list, value), "corrupt append");
    if (!wst_list_idx_int_remove(&list, 1))
        fail("corrupt remove");

    WstListIdxIntNode *nodes = (WstListIdxIntNode *)list.hdr.nodes;
    if (nodes == NULL)
        fail("corrupt nodes");

    LOG_I("corruption: live prev link points nowhere");
    nodes[3].hdr.prev = 0;
    wst_list_idx_int_dump(&list);
    nodes[3].hdr.prev = 1;

    LOG_I("corruption: live node dropped from both chains");
    uint32_t saved_next = nodes[3].hdr.next;
    uint32_t saved_prev = nodes[5].hdr.prev;
    nodes[3].hdr.next = 5;
    nodes[5].hdr.prev = 3;
    wst_list_idx_int_dump(&list);
    nodes[3].hdr.next = saved_next;
    nodes[5].hdr.prev = saved_prev;

    LOG_I("corruption: free head points at a live node");
    uint32_t saved_free = list.hdr.free;
    list.hdr.free = 1;
    wst_list_idx_int_dump(&list);
    list.hdr.free = saved_free;

    LOG_I("corruption: tail links back to head");
    uint32_t saved_tail_next = nodes[5].hdr.next;
    nodes[5].hdr.next = 1;
    wst_list_idx_int_dump(&list);
    nodes[5].hdr.next = saved_tail_next;

    require(wst_list_idx_int_free(&list), "corrupt free");
}

int main(int argc, char **argv)
{
    bool help = false;
    bool verbose = false;
    const char *log_dir = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &help,
            .short_name = "-h",
            .long_name = "--help",
            .description = "print this help and exit",
        },
#ifdef WST_DEBUG
        {
            .type = ARG_SWITCH,
            .dest = &verbose,
            .short_name = "-v",
            .long_name = "--verbose",
            .description = "enable verbose logging messages",
        },
#endif
        {
            .type = ARG_STRING,
            .dest = &log_dir,
            .short_name = "-l",
            .long_name = "--logdir",
            .description = "write a timestamped log into this directory",
        },
    };

    ArgParser parser = {
        .prog = argv[0],
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    if (!argparse_parse(&parser, argc, argv)) {
        /* Check if help flag was parsed, even if other required args are missing. */
        if (help)
            argparse_print_help(&parser);
        else
            fprintf(stderr, "%s, run '%s --help' for usage\n", parser.error, argv[0]);
        return EXIT_FAILURE;
    }

    if (help) {
        argparse_print_help(&parser);
        return EXIT_SUCCESS;
    }

    if (verbose)
        wst_log_set_max_prio(WST_LOG_VERBOSE);

    if (log_dir == NULL)
        log_dir = "logs";

    if (!wst_log_open(log_dir, WST_LOG_FORMAT_HTML)) {
        fprintf(stderr, "cannot open HTML log in '%s'\n", log_dir);
        return EXIT_FAILURE;
    }

    printf("HTML log: %s\n", wst_log_path());

    LOG_E("this is error log message");
    LOG_W("this is warning log message");
    LOG_I("this is info log message");
    LOG_D("this is debug log message");
    LOG_V("this is verbose log message");

    LOG_I("each dump below embeds a fresh graph");
    LOG_I("running pointer backend operations");
    pointer_demo();
    LOG_I("running pointer backend corruption");
    pointer_corruption();
    LOG_I("running index backend operations");
    index_demo();
    LOG_I("running index backend corruption");
    index_corruption();
    LOG_I("done");

    wst_log_close();
    return EXIT_SUCCESS;
}
