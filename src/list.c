#define LIST_IMPL
#include "toys/list.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Untyped list helpers shared by every typed list. */

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

WstListNodeHdr *wst_list_hdr_find(const WstListHdr *list, size_t data_off,
                                  const void *value, size_t value_size)
{
    assert(list != NULL);
    assert(value != NULL);
    assert(value_size > 0);

    for (WstListNodeHdr *node = list->head; node != NULL; node = node->next) {
        const void *data = (const unsigned char *)node + data_off;
        if (memcmp(data, value, value_size) == 0)
            return node;
    }
    return NULL;
}

ssize_t wst_list_hdr_find_index(const WstListHdr *list, size_t data_off,
                                const void *value, size_t value_size)
{
    assert(list != NULL);
    assert(value != NULL);
    assert(value_size > 0);

    size_t idx = 0;
    for (WstListNodeHdr *node = list->head; node != NULL;
         node = node->next, idx++) {
        const void *data = (const unsigned char *)node + data_off;
        if (memcmp(data, value, value_size) == 0)
            return (ssize_t)idx;
    }
    return -1;
}

WstListErr wst_list_err_from_vec(WstVecErr err)
{
    switch (err) {
    case WST_VEC_NO_ERR:
        return WST_LIST_NO_ERR;
    case WST_VEC_ERR_NOMEM:
        return WST_LIST_ERR_NOMEM;
    case WST_VEC_ERR_EMPTY:
        return WST_LIST_ERR_EMPTY;
    case WST_VEC_ERR_NULL:
        return WST_LIST_ERR_NULL;
    case WST_VEC_ERR_CORRUPT:
        return WST_LIST_ERR_CORRUPT;
    case WST_VEC_ERR_OVERFLOW:
        return WST_LIST_ERR_OVERFLOW;
    }
    return WST_LIST_ERR_CORRUPT;
}

const char *wst_list_slot_state_name(WstListSlotState state)
{
    switch (state) {
    case WST_LIST_SLOT_LIVE:
        return "live";
    case WST_LIST_SLOT_FREE:
        return "free";
    case WST_LIST_SLOT_ORPHAN:
    default:
        return "orphan";
    }
}

WstListSlotCounts wst_list_slot_counts(const WstListSlotState *state,
                                       size_t count)
{
    WstListSlotCounts counts = { 0 };
    if (state == NULL)
        return counts;

    for (size_t i = 0; i < count; i++) {
        switch (state[i]) {
        case WST_LIST_SLOT_LIVE:
            counts.live++;
            break;
        case WST_LIST_SLOT_FREE:
            counts.pooled++;
            break;
        case WST_LIST_SLOT_ORPHAN:
        default:
            counts.orphan++;
            break;
        }
    }

    return counts;
}

void wst_list_diag_add(WstListDiag *diag, const char *fmt, ...)
{
    if (diag == NULL || diag->stream == NULL || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    vfprintf(diag->stream, fmt, args);
    va_end(args);
    fputc('\n', diag->stream);
    diag->count++;
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
