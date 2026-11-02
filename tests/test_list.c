#include "tests_common.h"
#include "log_util.h"

#include "toys/list.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char payload[128];
} ElemBig;

#define LIST_ELEM ElemBig
#define LIST_T WstListBig
#define LIST_F(name) wst_list_big_ ## name
#define LIST_REF
#define LIST_IMPL
#include "toys/impl/list_internal.h"
#undef LIST_IMPL

static size_t block_capacity(const WstListInt *list)
{
    size_t cap = 0;
    for (size_t i = 0; i < list->blocks.length; i++)
        cap += list->blocks.data[i]->cap;
    return cap;
}

static void test_append_get(void)
{
    WstListInt list = { 0 };

    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "zero list verifies");
    CHECK(list.hdr.head == NULL && list.hdr.tail == NULL && list.hdr.free == NULL,
          "zero list is empty");

    int v = 0;
    CHECK(!wst_list_int_get(&list, 0, &v), "get on empty fails");
    CHECK(wst_list_int_pop(&list, &v) == WST_LIST_ERR_EMPTY, "pop empty errors");

    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");
    CHECK(list.hdr.head == NULL && list.blocks.length == 0, "init leaves empty list");
    CHECK(list.blocks.cap >= 1, "init reserves block pointer storage");

    for (int i = 0; i < 100; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append %d", i);

    CHECK(list.blocks.length == 4, "100 nodes use four blocks");
    CHECK(list.blocks.data[0]->cap == WST_LIST_BLOCK_INIT_CAP, "first block size");
    for (size_t i = 1; i < list.blocks.length; i++)
        CHECK(list.blocks.data[i]->cap == list.blocks.data[i - 1]->cap * 2,
              "block %zu doubles", i);
    CHECK(block_capacity(&list) >= 100, "block capacity covers nodes");

    for (int i = 0; i < 100; i++) {
        v = -1;
        CHECK(wst_list_int_get(&list, (size_t)i, &v) && v == i, "get %d", i);
    }
    CHECK(!wst_list_int_get(&list, 100, &v), "get past end fails");
    CHECK(list.hdr.head != NULL && list.hdr.tail != NULL, "endpoints set");
    CHECK(list.hdr.head->prev == NULL && list.hdr.tail->next == NULL,
          "endpoints are list ends");
    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "filled list verifies");

    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
    CHECK(list.hdr.head == NULL && list.hdr.tail == NULL && list.hdr.free == NULL,
          "free clears endpoints");
    CHECK(list.blocks.data == NULL && list.blocks.length == 0 && list.blocks.cap == 0,
          "free zeroes block vector");
    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "double free safe");
}

static void test_indexed_ops(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 10; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    int v = 0;
    CHECK(wst_list_int_set(&list, 5, 50), "set middle");
    CHECK(wst_list_int_get(&list, 5, &v) && v == 50, "set read back");
    CHECK(!wst_list_int_set(&list, 10, 1), "set past end fails");

    CHECK(wst_list_int_insert(&list, 0, -1) == WST_LIST_NO_ERR, "insert front");
    CHECK(wst_list_int_insert(&list, 5, 500) == WST_LIST_NO_ERR, "insert middle");
    CHECK(wst_list_int_insert(&list, 12, 999) == WST_LIST_NO_ERR, "insert back");
    CHECK(wst_list_int_insert(&list, 14, 0) == WST_LIST_ERR_BOUNDS,
          "insert past end fails");

    CHECK(wst_list_int_get(&list, 0, &v) && v == -1, "front value");
    CHECK(wst_list_int_get(&list, 5, &v) && v == 500, "middle value");
    CHECK(wst_list_int_get(&list, 12, &v) && v == 999, "back value");
    CHECK(wst_list_int_get(&list, 11, &v) && v == 9, "tail preserved");

    CHECK(wst_list_int_remove(&list, 0), "remove front");
    CHECK(wst_list_int_remove(&list, 4), "remove middle");
    CHECK(!wst_list_int_remove(&list, 100), "remove past end fails");
    CHECK(wst_list_int_get(&list, 0, &v) && v == 0, "new front value");
    CHECK(wst_list_int_get(&list, 100, &v) == false, "list shrank");

    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "mutated list verifies");
    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_node_ops(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 8; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    WstListIntNode *first = wst_list_int_get_node(&list, 0);
    WstListIntNode *last = wst_list_int_get_node(&list, 7);
    CHECK(first != NULL && first->data == 0, "first node");
    CHECK(last != NULL && last->data == 7, "last node");
    CHECK(wst_list_int_get_node(&list, 8) == NULL, "node past end");

    CHECK(wst_list_int_insert_by_node(&list, first, 100) == WST_LIST_NO_ERR,
          "insert after node");
    CHECK(wst_list_int_insert_by_node(&list, NULL, -5) == WST_LIST_NO_ERR,
          "insert by null prepends");
    CHECK(wst_list_int_insert_by_node(&list, last, 200) == WST_LIST_NO_ERR,
          "insert after tail");

    int v = 0;
    CHECK(wst_list_int_get(&list, 0, &v) && v == -5, "prepended value");
    CHECK(wst_list_int_get(&list, 2, &v) && v == 100, "inserted value");
    CHECK(wst_list_int_get(&list, 10, &v) && v == 200, "appended after tail");

    CHECK(wst_list_int_remove_node(&list, first), "remove by node");
    CHECK(!wst_list_int_remove_node(&list, NULL), "remove null node fails");
    CHECK(wst_list_int_find(&list, 0) == -1, "removed value is gone");

    /* Pointers stay stable when later inserts allocate new blocks. */
    WstListIntNode *stable = wst_list_int_get_node(&list, 0);
    CHECK(stable != NULL && stable->data == -5, "stable node value");
    for (int i = 0; i < 1000; i++)
        CHECK(wst_list_int_append(&list, 1000 + i) == WST_LIST_NO_ERR, "grow");
    CHECK(stable->data == -5, "node pointer survives block growth");
    CHECK(wst_list_int_remove_node(&list, stable), "remove stable node");
    CHECK(wst_list_int_find(&list, -5) == -1, "stable value removed");

    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "node ops verify");
    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_find(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");
    CHECK(wst_list_int_find(&list, 3) == -1, "find in empty list");
    CHECK(wst_list_int_find_node(&list, 3) == NULL, "find node in empty list");

    int values[] = { 1, 2, 3, 2 };
    for (size_t i = 0; i < ARR_LEN(values); i++)
        CHECK(wst_list_int_append(&list, values[i]) == WST_LIST_NO_ERR, "append");

    CHECK(wst_list_int_find(&list, 2) == 1, "find returns first match");
    CHECK(wst_list_int_find(&list, 9) == -1, "find missing value");

    WstListIntNode *node = wst_list_int_find_node(&list, 3);
    CHECK(node != NULL && node->data == 3, "find node returns match");
    CHECK(wst_list_int_find_node(&list, 9) == NULL, "find missing node");

    CHECK(wst_list_int_set(&list, 1, 9), "set duplicate");
    CHECK(wst_list_int_find(&list, 9) == 1, "find updated value");
    CHECK(wst_list_int_find(&list, 2) == 3, "old duplicate remains at tail");

    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_pop(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 20; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    bool ok = true;
    for (int i = 19; i >= 0; i--) {
        int v = -1;
        if (wst_list_int_pop(&list, &v) != WST_LIST_NO_ERR || v != i)
            ok = false;
    }
    CHECK(ok, "pop returns values in reverse order");

    CHECK(wst_list_int_append(&list, 42) == WST_LIST_NO_ERR, "append before discard");
    CHECK(wst_list_int_pop(&list, NULL) == WST_LIST_NO_ERR, "pop with null out");
    CHECK(wst_list_int_pop(&list, NULL) == WST_LIST_ERR_EMPTY, "discarded value is gone");
    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "popped list verifies");
    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_clear_reinit(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 100; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    size_t blocks = list.blocks.length;
    size_t cap = block_capacity(&list);
    CHECK(wst_list_int_clear(&list) == WST_LIST_NO_ERR, "clear ok");
    CHECK(list.hdr.head == NULL && list.hdr.tail == NULL, "clear empties list");
    CHECK(list.hdr.free != NULL, "clear pools nodes");
    CHECK(list.blocks.length == blocks && block_capacity(&list) == cap,
          "clear keeps blocks");
    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "cleared list verifies");

    int v = 0;
    CHECK(wst_list_int_pop(&list, &v) == WST_LIST_ERR_EMPTY, "pop after clear errors");
    for (int i = 0; i < 20; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append after clear");
    CHECK(list.blocks.length == blocks, "append after clear reuses nodes");
    CHECK(wst_list_int_get(&list, 19, &v) && v == 19, "reused values");

    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "reinit ok");
    CHECK(list.hdr.head == NULL && list.hdr.free == NULL, "reinit empties list");
    CHECK(list.blocks.length == 0 && list.blocks.cap >= 1, "reinit drops blocks");
    CHECK(!wst_list_int_get(&list, 0, &v), "reinit drops values");

    CHECK(wst_list_int_append(&list, 77) == WST_LIST_NO_ERR, "append after reinit");
    CHECK(wst_list_int_get(&list, 0, &v) && v == 77, "value after reinit");
    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_corrupt(void)
{
    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "init ok");
    for (int i = 0; i < 3; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "append");

#if VEC_USE_CANARY
    unsigned char *tail = (unsigned char *)list.blocks.data +
                          list.blocks.cap * sizeof(*list.blocks.data);
    unsigned char saved = tail[0];
    tail[0] ^= 0xFF;

    wst_list_int_dump(&list);
    CHECK(wst_list_int_verify(&list) == WST_LIST_ERR_CORRUPT, "damaged canary fails");
    CHECK(wst_list_int_append(&list, 4) == WST_LIST_ERR_CORRUPT,
          "append refused when corrupt");
    CHECK(wst_list_int_clear(&list) == WST_LIST_ERR_CORRUPT, "clear refused when corrupt");
    CHECK(wst_list_int_free(&list) == WST_LIST_ERR_CORRUPT, "free refused when corrupt");

    tail[0] = saved;
    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "restored canary verifies");
#endif

    size_t length = list.blocks.length;
    list.blocks.length = list.blocks.cap + 1;
    wst_list_int_dump(&list);
    CHECK(wst_list_int_verify(&list) == WST_LIST_ERR_CORRUPT, "bad length fails");
    CHECK(wst_list_int_pop(&list, NULL) == WST_LIST_ERR_CORRUPT,
          "pop refused when corrupt");
    list.blocks.length = length;
    CHECK(wst_list_int_verify(&list) == WST_LIST_NO_ERR, "restored length verifies");

    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "free ok after restore");
}

static void test_big_ref(void)
{
    WstListBig list = { 0 };
    CHECK(wst_list_big_init(&list) == WST_LIST_NO_ERR, "big init");

    for (int i = 0; i < 10; i++) {
        ElemBig v = { .id = i };
        memset(v.payload, i, sizeof(v.payload));
        CHECK(wst_list_big_append(&list, &v) == WST_LIST_NO_ERR, "big append");
    }

    ElemBig v = { 0 };
    CHECK(wst_list_big_get(&list, 5, &v) && v.id == 5, "big get");
    CHECK(v.payload[0] == 5 && v.payload[127] == 5, "big payload intact");

    ElemBig find = { .id = 7 };
    memset(find.payload, 7, sizeof(find.payload));
    CHECK(wst_list_big_find(&list, &find) == 7, "big find");
    WstListBigNode *node = wst_list_big_find_node(&list, &find);
    CHECK(node != NULL && node->data.id == 7, "big find node");

    ElemBig setv = { .id = -7 };
    memset(setv.payload, 0x5A, sizeof(setv.payload));
    CHECK(wst_list_big_set(&list, 7, &setv), "big set");
    CHECK(wst_list_big_find(&list, &setv) == 7, "big set read back");

    ElemBig ins = { .id = 99 };
    memset(ins.payload, 0x33, sizeof(ins.payload));
    CHECK(wst_list_big_insert(&list, 0, &ins) == WST_LIST_NO_ERR, "big insert");
    CHECK(wst_list_big_pop(&list, &v) == WST_LIST_NO_ERR && v.id == 9,
          "big pop");

    CHECK(wst_list_big_verify(&list) == WST_LIST_NO_ERR, "big verifies");
    CHECK(wst_list_big_free(&list) == WST_LIST_NO_ERR, "big free");
}

static void test_verify_graceful(void)
{
    CHECK(wst_list_int_verify(NULL) == WST_LIST_ERR_NULL, "null list");
    wst_list_int_dump(NULL);

    WstListInt empty = { 0 };
    wst_list_int_dump(&empty);
    CHECK(wst_list_int_verify(&empty) == WST_LIST_NO_ERR, "empty dumps and verifies");
    CHECK(wst_list_int_clear(&empty) == WST_LIST_NO_ERR, "clear empty list");
    CHECK(wst_list_int_free(&empty) == WST_LIST_NO_ERR, "free empty list");
}

static void test_err_str(void)
{
    WstListErr codes[] = {
        WST_LIST_NO_ERR, WST_LIST_ERR_NOMEM, WST_LIST_ERR_EMPTY,
        WST_LIST_ERR_NULL, WST_LIST_ERR_CORRUPT, WST_LIST_ERR_OVERFLOW,
        WST_LIST_ERR_BOUNDS,
    };
    for (size_t i = 0; i < ARR_LEN(codes); i++)
        CHECK(wst_list_err_str(codes[i]) != NULL, "err str %zu", i);
    CHECK(strcmp(wst_list_err_str(WST_LIST_NO_ERR), "no error") == 0, "err str text");
}

static void test_dump_html(void)
{
    char *dir = test_tmpdir("wst-list-html");
    CHECK(dir != NULL, "dump dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open dump log");
    wst_log_set_max_prio(WST_LOG_VERBOSE);

    WstListInt list = { 0 };
    CHECK(wst_list_int_init(&list) == WST_LIST_NO_ERR, "dump init");
    for (int i = 0; i < 20; i++)
        CHECK(wst_list_int_append(&list, i) == WST_LIST_NO_ERR, "dump append");
    CHECK(wst_list_int_remove(&list, 3), "dump remove");

    wst_list_int_dump(&list);

    char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
    wst_log_close();

    char *data = path != NULL ? test_read_file(path) : NULL;
    CHECK(data != NULL, "read dump log");
    if (data != NULL) {
        CHECK(strstr(data, "list WstListInt @") != NULL, "dump header");
        CHECK(strstr(data, "block[0]") != NULL, "first block");
        CHECK(strstr(data, "block[1]") != NULL, "second block");
        CHECK(strstr(data, "slot  state") != NULL, "slot table header");
        CHECK(strstr(data, "  live     19") != NULL, "live count");
        CHECK(strstr(data, "  pooled   5") != NULL, "pooled count");
        CHECK(strstr(data, "  [  0] live") != NULL, "live row");
        CHECK(strstr(data, "  [  3] free") != NULL, "free row");

        CHECK(strstr(data, "<span class=\"image-wrap\">") != NULL,
              "dump graph scroll wrapper");
        CHECK(strstr(data, "<img class=\"log-image\"") != NULL,
              "dump graph image");
        CHECK(strstr(data, ".svg\"") != NULL, "dump graph path");
    }

    char *dot_path = test_path_join(dir, "list.dot");
    char *dot = dot_path != NULL ? test_read_file(dot_path) : NULL;
    CHECK(dot != NULL && strstr(dot, "blocks=2") != NULL,
          "graph header fields");
    CHECK(dot != NULL && strstr(dot, "data=19") != NULL, "graph data");
    CHECK(dot != NULL && strstr(dot, "next=") != NULL, "graph next field");
    CHECK(dot != NULL && strstr(dot, "prev=") != NULL, "graph prev field");
    CHECK(dot != NULL && strstr(dot, "subgraph \"cluster_b0\"") != NULL,
          "graph block cluster");
    CHECK(dot != NULL && strstr(dot, "bgcolor=\"#1c1b1f\"") != NULL,
          "dark graph background");
    CHECK(dot != NULL && strstr(dot, "shape=record") != NULL,
          "graph card nodes");
    CHECK(dot != NULL && strstr(dot, "fillcolor=\"#4a4458\"") != NULL,
          "live graph style");
    free(dot);
    free(dot_path);

    CHECK(wst_list_int_free(&list) == WST_LIST_NO_ERR, "dump free");
    free(data);
    free(path);
    test_rm_rf(dir);
    free(dir);
}

int main(void)
{
    test_append_get();
    test_indexed_ops();
    test_node_ops();
    test_find();
    test_pop();
    test_clear_reinit();
    test_corrupt();
    test_big_ref();
    test_verify_graceful();
    test_err_str();
    test_dump_html();
    return tests_summary();
}
