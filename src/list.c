#define LIST_IMPL
#include "toys/list.h"

/* Node-header-only operations, shared by every typed list. */

WstListNodeHdr *wst_list_hdr_at(const WstListHdr *list, size_t idx)
{
    assert(list != NULL);

    WstListNodeHdr *node = list->head;
    while (node != NULL && idx > 0) {
        node = node->next;
        idx--;
    }
    return node;
}

void wst_list_hdr_insert_after(WstListHdr *list, WstListNodeHdr *prev,
                               WstListNodeHdr *node)
{
    assert(list != NULL);
    assert(node != NULL);

    node->prev = prev;

    if (prev != NULL) {
        node->next = prev->next;
        prev->next = node;
    } else {
        node->next = list->head;
        list->head = node;
    }

    if (node->next != NULL)
        node->next->prev = node;
    else
        list->tail = node;
}

void wst_list_hdr_remove(WstListHdr *list, WstListNodeHdr *node)
{
    assert(list != NULL);
    assert(node != NULL);

    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    node->next = list->free;
    node->prev = NULL;
    list->free = node;
}

WstListNodeHdr *wst_list_hdr_take_free(WstListHdr *list)
{
    assert(list != NULL);

    WstListNodeHdr *node = list->free;
    if (node != NULL) {
        list->free = node->next;
        node->next = NULL;
        node->prev = NULL;
    }
    return node;
}

void wst_list_hdr_reset(WstListHdr *list)
{
    assert(list != NULL);

    list->head = NULL;
    list->tail = NULL;
    list->free = NULL;
}

void wst_list_hdr_add_free(WstListHdr *list, void *nodes, size_t count,
                           size_t stride)
{
    assert(list != NULL);
    assert(nodes != NULL || count == 0);
    assert(stride >= sizeof(WstListNodeHdr));

    unsigned char *base = nodes;

    for (size_t i = count; i > 0; i--) {
        WstListNodeHdr *node = (WstListNodeHdr *)(base + (i - 1) * stride);
        node->next = list->free;
        node->prev = NULL;
        list->free = node;
    }
}

const char *wst_list_err_str(WstListErr err)
{
    switch (err) {
    case WST_LIST_NO_ERR:
        return "no error";
    case WST_LIST_ERR_NOMEM:
        return "out of memory";
    case WST_LIST_ERR_EMPTY:
        return "list is empty";
    case WST_LIST_ERR_NULL:
        return "null list";
    case WST_LIST_ERR_CORRUPT:
        return "list is corrupt";
    case WST_LIST_ERR_OVERFLOW:
        return "capacity overflow";
    case WST_LIST_ERR_BOUNDS:
        return "index out of bounds";
    default:
        return "unknown error";
    }
}
