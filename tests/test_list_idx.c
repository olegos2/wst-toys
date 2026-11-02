#include "tests_common.h"
#include "log_util.h"

#include "toys/impl/list_idx_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LIST_ELEM int
#define LIST_T WstListIdxInt
#define LIST_F(name) wst_list_idx_int_ ## name
#define LIST_FMT "%d"
#define LIST_IMPL
#include "toys/impl/list_idx_internal.h"
#undef LIST_IMPL

typedef struct {
    int id;
    char payload[128];
} ElemBig;

#define LIST_ELEM ElemBig
#define LIST_T WstListIdxBig
#define LIST_F(name) wst_list_idx_big_ ## name
#define LIST_REF
#define LIST_IMPL
#include "toys/impl/list_idx_internal.h"
#undef LIST_IMPL

static WstListIdxIntNode *int_nodes(WstListIdxInt *list)
{
    return (WstListIdxIntNode *)list->hdr.nodes;
}

static void test_lifecycle(void)
{
    WstListIdxInt list = { 0 };

    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "zero list verifies");
    CHECK(list.hdr.nodes == NULL && list.hdr.cap == 0 && list.hdr.len == 0,
          "zero list has no storage");
    CHECK(list.hdr.free == WST_LISTI_NIL, "zero list has no free list");

    int v = 0;
    CHECK(!wst_list_idx_int_get(&list, 0, &v), "get on empty fails");
    CHECK(wst_list_idx_int_pop(&list, &v) == WST_LIST_ERR_EMPTY,
          "pop empty errors");

    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");
    CHECK(list.hdr.nodes == NULL && list.hdr.cap == 0, "init leaves list empty");

    CHECK(wst_list_idx_int_insert(&list, 0, 7) == WST_LIST_NO_ERR,
          "insert into empty list");
    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == 7, "inserted value");
    CHECK(wst_list_idx_int_pop(&list, &v) == WST_LIST_NO_ERR && v == 7,
          "pop only node");
    CHECK(int_nodes(&list)[0].hdr.next == WST_LISTI_NIL &&
          int_nodes(&list)[0].hdr.prev == WST_LISTI_NIL,
          "sentinel is empty after pop");
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "emptied list verifies");

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR,
          "double free safe");
}

static void test_append_get(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    CHECK(wst_list_idx_int_append(&list, 0) == WST_LIST_NO_ERR, "first append");
    CHECK(list.hdr.cap == WST_LISTI_INIT_CAP, "first append uses initial capacity");
    CHECK(list.hdr.len == 1, "one real slot is allocated");

    for (int i = 1; i < 100; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR,
              "append %d", i);

    CHECK(list.hdr.len == 100, "all real slots are allocated");
    CHECK(list.hdr.cap == 128, "node array grew geometrically");
    CHECK(list.hdr.nodes != NULL && int_nodes(&list)[0].hdr.next != WST_LISTI_NIL &&
          int_nodes(&list)[0].hdr.prev != WST_LISTI_NIL,
          "sentinel links both ends");
    CHECK(list.hdr.free == WST_LISTI_NIL, "no free slots");

    int v = 0;
    for (int i = 0; i < 100; i++)
        CHECK(wst_list_idx_int_get(&list, (size_t)i, &v) && v == i,
              "get %d", i);
    CHECK(!wst_list_idx_int_get(&list, 100, &v), "get past end fails");
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "filled list verifies");

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
    CHECK(list.hdr.nodes == NULL && list.hdr.cap == 0 && list.hdr.len == 0,
          "free zeroes storage");
}

static void test_indexed_ops(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 10; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    int v = 0;
    CHECK(wst_list_idx_int_set(&list, 5, 50), "set middle");
    CHECK(wst_list_idx_int_get(&list, 5, &v) && v == 50, "set read back");
    CHECK(!wst_list_idx_int_set(&list, 10, 1), "set past end fails");

    CHECK(wst_list_idx_int_insert(&list, 0, -1) == WST_LIST_NO_ERR,
          "insert front");
    CHECK(wst_list_idx_int_insert(&list, 5, 500) == WST_LIST_NO_ERR,
          "insert middle");
    CHECK(wst_list_idx_int_insert(&list, 12, 999) == WST_LIST_NO_ERR,
          "insert back");
    CHECK(wst_list_idx_int_insert(&list, 14, 0) == WST_LIST_ERR_BOUNDS,
          "insert past end fails");

    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == -1, "front value");
    CHECK(wst_list_idx_int_get(&list, 5, &v) && v == 500, "middle value");
    CHECK(wst_list_idx_int_get(&list, 12, &v) && v == 999, "back value");
    CHECK(wst_list_idx_int_get(&list, 11, &v) && v == 9, "tail preserved");

    CHECK(wst_list_idx_int_remove(&list, 0), "remove front");
    CHECK(wst_list_idx_int_remove(&list, 4), "remove middle");
    CHECK(!wst_list_idx_int_remove(&list, 100), "remove past end fails");
    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == 0, "new front value");
    CHECK(!wst_list_idx_int_get(&list, 12, &v), "list shrank");

    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "mutated list verifies");
    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_node_ops(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 8; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    WstListIdxIntNode *first = wst_list_idx_int_get_node(&list, 0);
    CHECK(first != NULL && first->data == 0, "first node");
    CHECK(wst_list_idx_int_get_node(&list, 8) == NULL, "node past end");

    /* This insert grows the array, so node pointers must be fetched again. */
    CHECK(wst_list_idx_int_insert_by_node(&list, first, 100) ==
          WST_LIST_NO_ERR, "insert after node");
    first = wst_list_idx_int_get_node(&list, 0);
    WstListIdxIntNode *last = wst_list_idx_int_get_node(&list, 8);
    CHECK(first != NULL && first->data == 0, "first node after growth");
    CHECK(last != NULL && last->data == 7, "last node after growth");

    CHECK(wst_list_idx_int_insert_by_node(&list, NULL, -5) ==
          WST_LIST_NO_ERR, "insert by null prepends");
    CHECK(wst_list_idx_int_insert_by_node(&list, last, 200) ==
          WST_LIST_NO_ERR, "insert after tail");

    int v = 0;
    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == -5, "prepended value");
    CHECK(wst_list_idx_int_get(&list, 2, &v) && v == 100, "inserted value");
    CHECK(wst_list_idx_int_get(&list, 10, &v) && v == 200,
          "appended after tail");

    CHECK(wst_list_idx_int_remove_node(&list, first), "remove by node");
    CHECK(!wst_list_idx_int_remove_node(&list, NULL), "remove null node fails");
    CHECK(!wst_list_idx_int_remove_node(&list, &int_nodes(&list)[0]),
          "remove sentinel fails");
    CHECK(wst_list_idx_int_find(&list, 0) == -1, "removed value is gone");

    for (int i = 0; i < 1000; i++)
        CHECK(wst_list_idx_int_append(&list, 1000 + i) == WST_LIST_NO_ERR,
              "grow");
    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == -5,
          "front survives growth");
    CHECK(wst_list_idx_int_get(&list, 9, &v) && v == 200,
          "tail survives growth");
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "node ops verify");

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_find(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");
    CHECK(wst_list_idx_int_find(&list, 3) == -1, "find in empty list");
    CHECK(wst_list_idx_int_find_node(&list, 3) == NULL,
          "find node in empty list");

    int values[] = { 1, 2, 3, 2 };
    for (size_t i = 0; i < ARR_LEN(values); i++)
        CHECK(wst_list_idx_int_append(&list, values[i]) == WST_LIST_NO_ERR,
              "append");

    CHECK(wst_list_idx_int_find(&list, 2) == 1, "find returns first match");
    CHECK(wst_list_idx_int_find(&list, 9) == -1, "find missing value");

    WstListIdxIntNode *node = wst_list_idx_int_find_node(&list, 3);
    CHECK(node != NULL && node->data == 3, "find node returns match");
    CHECK(wst_list_idx_int_find_node(&list, 9) == NULL,
          "find missing node");

    CHECK(wst_list_idx_int_set(&list, 1, 9), "set duplicate");
    CHECK(wst_list_idx_int_find(&list, 9) == 1, "find updated value");
    CHECK(wst_list_idx_int_find(&list, 2) == 3,
          "old duplicate remains at tail");

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_pop(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 20; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    bool ok = true;
    for (int i = 19; i >= 0; i--) {
        int v = -1;
        if (wst_list_idx_int_pop(&list, &v) != WST_LIST_NO_ERR || v != i)
            ok = false;
    }
    CHECK(ok, "pop returns values in reverse order");
    CHECK(list.hdr.free != WST_LISTI_NIL, "popped nodes are pooled");
    CHECK(int_nodes(&list)[0].hdr.next == WST_LISTI_NIL &&
          int_nodes(&list)[0].hdr.prev == WST_LISTI_NIL,
          "pop empties the list");

    CHECK(wst_list_idx_int_append(&list, 42) == WST_LIST_NO_ERR,
          "append before discard");
    CHECK(wst_list_idx_int_pop(&list, NULL) == WST_LIST_NO_ERR,
          "pop with null out");
    CHECK(wst_list_idx_int_pop(&list, NULL) == WST_LIST_ERR_EMPTY,
          "discarded value is gone");
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "popped list verifies");
    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_clear_reinit(void)
{
    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "init ok");

    for (int i = 0; i < 100; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    uint32_t len = list.hdr.len;
    size_t cap = list.hdr.cap;
    CHECK(wst_list_idx_int_clear(&list) == WST_LIST_NO_ERR, "clear ok");
    CHECK(int_nodes(&list)[0].hdr.next == WST_LISTI_NIL &&
          int_nodes(&list)[0].hdr.prev == WST_LISTI_NIL, "clear empties list");
    CHECK(list.hdr.free != WST_LISTI_NIL, "clear pools nodes");
    CHECK(list.hdr.len == len && list.hdr.cap == cap, "clear keeps storage");
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "cleared list verifies");

    int v = 0;
    CHECK(wst_list_idx_int_pop(&list, &v) == WST_LIST_ERR_EMPTY,
          "pop after clear errors");
    for (int i = 0; i < 20; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR,
              "append after clear");
    CHECK(list.hdr.len == len, "append after clear reuses slots");
    CHECK(wst_list_idx_int_get(&list, 19, &v) && v == 19, "reused values");

    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "reinit ok");
    CHECK(list.hdr.nodes == NULL && list.hdr.cap == 0 && list.hdr.len == 0,
          "reinit drops storage");
    CHECK(list.hdr.free == WST_LISTI_NIL, "reinit empties free list");
    CHECK(!wst_list_idx_int_get(&list, 0, &v), "reinit drops values");

    CHECK(wst_list_idx_int_append(&list, 77) == WST_LIST_NO_ERR,
          "append after reinit");
    CHECK(wst_list_idx_int_get(&list, 0, &v) && v == 77,
          "value after reinit");
    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_corrupt(void)
{
    CHECK(wst_list_idx_int_verify(NULL) == WST_LIST_ERR_NULL, "null list");
    wst_list_idx_int_dump(NULL);

    WstListIdxInt list = { 0 };
    wst_list_idx_int_dump(&list);
    CHECK(wst_list_idx_int_clear(&list) == WST_LIST_NO_ERR,
          "clear empty list");
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR,
          "init empty list");
    for (int i = 0; i < 3; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR, "append");

    wst_list_idx_int_dump(&list);

    int_nodes(&list)[0].hdr.next = WST_LISTI_NIL;
    wst_list_idx_int_dump(&list);
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "missing head fails");
    WstListIdxIntNode dummy = { 0 };
    CHECK(!wst_list_idx_int_remove_node(&list, &dummy),
          "remove refused when corrupt");
    CHECK(wst_list_idx_int_append(&list, 4) == WST_LIST_ERR_CORRUPT,
          "append refused when corrupt");
    CHECK(wst_list_idx_int_clear(&list) == WST_LIST_ERR_CORRUPT,
          "clear refused when corrupt");
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_ERR_CORRUPT,
          "init refused when corrupt");
    CHECK(wst_list_idx_int_free(&list) == WST_LIST_ERR_CORRUPT,
          "free refused when corrupt");
    CHECK(list.hdr.nodes != NULL, "refused free keeps storage");
    int_nodes(&list)[0].hdr.next = 1;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "restored head verifies");

    int_nodes(&list)[0].hdr.next = list.hdr.len + 1;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "head past len fails");
    int_nodes(&list)[0].hdr.next = 1;

    list.hdr.len = (uint32_t)list.hdr.cap + 1;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "bad length fails");
    CHECK(wst_list_idx_int_pop(&list, NULL) == WST_LIST_ERR_CORRUPT,
          "pop refused when corrupt");
    list.hdr.len = 3;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_NO_ERR,
          "restored length verifies");

    WstListIdxIntNode *nodes = list.hdr.nodes;
    list.hdr.nodes = NULL;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "missing node array fails");
    list.hdr.nodes = nodes;

    size_t cap = list.hdr.cap;
    list.hdr.cap = 0;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "zero capacity with storage fails");
    list.hdr.cap = cap;

#if SIZE_MAX > UINT32_MAX
    list.hdr.cap = (size_t)UINT32_MAX + 1;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "oversized capacity fails");
    list.hdr.cap = cap;
#endif

    list.hdr.free = 4;
    CHECK(wst_list_idx_int_verify(&list) == WST_LIST_ERR_CORRUPT,
          "free link past len fails");
    list.hdr.free = WST_LISTI_NIL;

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "free ok");
}

static void test_big_ref(void)
{
    WstListIdxBig list = { 0 };
    CHECK(wst_list_idx_big_verify(&list) == WST_LIST_NO_ERR,
          "zero big list verifies");
    CHECK(wst_list_idx_big_init(&list) == WST_LIST_NO_ERR, "big init");

    for (int i = 0; i < 10; i++) {
        ElemBig v = { .id = i };
        memset(v.payload, i, sizeof(v.payload));
        CHECK(wst_list_idx_big_append(&list, &v) == WST_LIST_NO_ERR,
              "big append");
    }

    ElemBig v = { 0 };
    CHECK(wst_list_idx_big_get(&list, 5, &v) && v.id == 5, "big get");
    CHECK(v.payload[0] == 5 && v.payload[127] == 5, "big payload intact");

    ElemBig find = { .id = 7 };
    memset(find.payload, 7, sizeof(find.payload));
    CHECK(wst_list_idx_big_find(&list, &find) == 7, "big find");
    WstListIdxBigNode *node = wst_list_idx_big_find_node(&list, &find);
    CHECK(node != NULL && node->data.id == 7, "big find node");

    ElemBig setv = { .id = -7 };
    memset(setv.payload, 0x5A, sizeof(setv.payload));
    CHECK(wst_list_idx_big_set(&list, 7, &setv), "big set");
    CHECK(wst_list_idx_big_find(&list, &setv) == 7, "big set read back");

    ElemBig ins = { .id = 99 };
    memset(ins.payload, 0x33, sizeof(ins.payload));
    CHECK(wst_list_idx_big_insert(&list, 0, &ins) == WST_LIST_NO_ERR,
          "big insert");
    CHECK(wst_list_idx_big_pop(&list, &v) == WST_LIST_NO_ERR && v.id == 9,
          "big pop");

    CHECK(wst_list_idx_big_verify(&list) == WST_LIST_NO_ERR, "big verifies");
    CHECK(wst_list_idx_big_free(&list) == WST_LIST_NO_ERR, "big free");
}

static void test_dump_html(void)
{
    char *dir = test_tmpdir("wst-list-idx-html");
    CHECK(dir != NULL, "dump dir");
    if (dir == NULL)
        return;

    CHECK(wst_log_open(dir, WST_LOG_FORMAT_HTML), "open dump log");
    wst_log_set_max_prio(WST_LOG_VERBOSE);

    WstListIdxInt list = { 0 };
    CHECK(wst_list_idx_int_init(&list) == WST_LIST_NO_ERR, "dump init");
    for (int i = 0; i < 20; i++)
        CHECK(wst_list_idx_int_append(&list, i) == WST_LIST_NO_ERR,
              "dump append");
    CHECK(wst_list_idx_int_remove(&list, 3), "dump remove");

    wst_list_idx_int_dump(&list);

    char *path = wst_log_path() != NULL ? strdup(wst_log_path()) : NULL;
    wst_log_close();

    char *data = path != NULL ? test_read_file(path) : NULL;
    CHECK(data != NULL, "read dump log");
    if (data != NULL) {
        CHECK(strstr(data, "list WstListIdxInt @") != NULL, "dump header");
        CHECK(strstr(data, "sentinel") == NULL, "no sentinel row");
        CHECK(strstr(data, "idx  state") != NULL, "slot table header");
        CHECK(strstr(data, "  live     19") != NULL, "live count");
        CHECK(strstr(data, "  pooled   1") != NULL, "pooled count");
        CHECK(strstr(data, "  [  1] live") != NULL, "live row");
        CHECK(strstr(data, "  [  4] free") != NULL, "free row");

        CHECK(strstr(data, "<span class=\"image-wrap\">") != NULL,
              "dump graph scroll wrapper");
        CHECK(strstr(data, "<img class=\"log-image\"") != NULL,
              "dump graph image");
        CHECK(strstr(data, ".svg\"") != NULL, "dump graph path");
    }

    char *dot_path = test_path_join(dir, "list.dot");
    char *dot = dot_path != NULL ? test_read_file(dot_path) : NULL;
    CHECK(dot != NULL && strstr(dot, "len=20") != NULL,
          "graph header fields");
    CHECK(dot != NULL && strstr(dot, "data=19") != NULL, "graph data");
    CHECK(dot != NULL && strstr(dot, "next=") != NULL, "graph next field");
    CHECK(dot != NULL && strstr(dot, "prev=") != NULL, "graph prev field");
    CHECK(dot != NULL && strstr(dot, "bgcolor=\"#1c1b1f\"") != NULL,
          "dark graph background");
    CHECK(dot != NULL && strstr(dot, "shape=record") != NULL,
          "graph card nodes");
    CHECK(dot != NULL && strstr(dot, "fillcolor=\"#4a4458\"") != NULL,
          "live graph style");
    free(dot);
    free(dot_path);

    CHECK(wst_list_idx_int_free(&list) == WST_LIST_NO_ERR, "dump free");
    free(data);
    free(path);
    test_rm_rf(dir);
    free(dir);
}

int main(void)
{
    test_lifecycle();
    test_append_get();
    test_indexed_ops();
    test_node_ops();
    test_find();
    test_pop();
    test_clear_reinit();
    test_corrupt();
    test_big_ref();
    test_dump_html();
    return tests_summary();
}
