#define LIST_IMPL
#include "toys/impl/list_common_internal.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Untyped operations shared by every index-linked list stamp. */

static inline WstListiNodeHdr *index_node_at(const WstListiHdr *hdr,
                                             size_t stride, uint32_t link)
{
    assert(hdr != NULL);
    assert(hdr->nodes != NULL);
    assert(stride >= sizeof(WstListiNodeHdr));
    assert(link <= hdr->len);

    return (WstListiNodeHdr *)((unsigned char *)hdr->nodes +
                               (size_t)link * stride);
}

WstListiNodeHdr *wst_listi_hdr_at(const WstListiHdr *hdr, size_t stride,
                                  size_t idx)
{
    assert(hdr != NULL);
    assert(stride >= sizeof(WstListiNodeHdr));

    if (hdr->len == 0)
        return NULL;

    WstListiNodeHdr *node = index_node_at(hdr, stride, 0);
    uint32_t link = node->next;

    while (link != 0 && idx > 0) {
        node = index_node_at(hdr, stride, link);
        link = node->next;
        idx--;
    }

    return link == 0 ? NULL : index_node_at(hdr, stride, link);
}

uint32_t wst_listi_hdr_index_of(const WstListiHdr *hdr, size_t stride,
                                const WstListiNodeHdr *node)
{
    assert(hdr != NULL);
    assert(hdr->nodes != NULL);
    assert(node != NULL);
    assert(stride >= sizeof(WstListiNodeHdr));

    uintptr_t base = (uintptr_t)hdr->nodes;
    uintptr_t addr = (uintptr_t)node;
    assert(addr >= base);

    size_t diff = (size_t)(addr - base);
    assert(diff % stride == 0);

    size_t idx = diff / stride;
    assert(idx <= hdr->len);
    return (uint32_t)idx;
}

uint32_t wst_listi_hdr_tail(const WstListiHdr *hdr, size_t stride)
{
    assert(hdr != NULL);

    if (hdr->len == 0)
        return 0;

    return index_node_at(hdr, stride, 0)->prev;
}

WstListErr wst_listi_hdr_alloc(WstListiHdr *hdr, size_t stride,
                               size_t init_cap, uint32_t *out)
{
    assert(hdr != NULL);
    assert(out != NULL);

    if (stride < sizeof(WstListiNodeHdr))
        return WST_LIST_ERR_CORRUPT;

    if (hdr->free != 0) {
        uint32_t link = hdr->free;
        WstListiNodeHdr *node = index_node_at(hdr, stride, link);
        hdr->free = node->next;
        node->next = 0;
        node->prev = 0;
        *out = link;
        return WST_LIST_NO_ERR;
    }

    if (hdr->len == UINT32_MAX)
        return WST_LIST_ERR_OVERFLOW;

    if (hdr->len == hdr->cap) {
        if (hdr->cap >= UINT32_MAX)
            return WST_LIST_ERR_OVERFLOW;

        if (init_cap == 0)
            init_cap = 1;

        size_t cap;
        if (hdr->cap == 0)
            cap = init_cap;
        else if (hdr->cap > UINT32_MAX / 2)
            cap = UINT32_MAX;
        else
            cap = hdr->cap * 2;

        if (cap > UINT32_MAX)
            cap = UINT32_MAX;

        if (cap > SIZE_MAX / stride - 1)
            return WST_LIST_ERR_OVERFLOW;

        void *nodes = realloc(hdr->nodes, (cap + 1) * stride);
        if (nodes == NULL)
            return WST_LIST_ERR_NOMEM;

        hdr->nodes = nodes;
        hdr->cap = cap;

        if (hdr->len == 0) {
            WstListiNodeHdr *sentinel = index_node_at(hdr, stride, 0);
            sentinel->next = 0;
            sentinel->prev = 0;
        }
    }

    uint32_t link = hdr->len + 1;
    hdr->len = link;

    WstListiNodeHdr *node = index_node_at(hdr, stride, link);
    node->next = 0;
    node->prev = 0;
    *out = link;
    return WST_LIST_NO_ERR;
}

void wst_listi_hdr_insert_after(WstListiHdr *hdr, size_t stride,
                                uint32_t prev, uint32_t link)
{
    assert(hdr != NULL);
    assert(prev <= hdr->len);
    assert(link != 0 && link <= hdr->len);

    WstListiNodeHdr *prev_node = index_node_at(hdr, stride, prev);
    WstListiNodeHdr *node = index_node_at(hdr, stride, link);

    node->prev = prev;
    node->next = prev_node->next;
    index_node_at(hdr, stride, prev_node->next)->prev = link;
    prev_node->next = link;
}

void wst_listi_hdr_remove(WstListiHdr *hdr, size_t stride, uint32_t link)
{
    assert(hdr != NULL);
    assert(link != 0 && link <= hdr->len);

    WstListiNodeHdr *node = index_node_at(hdr, stride, link);

    index_node_at(hdr, stride, node->prev)->next = node->next;
    index_node_at(hdr, stride, node->next)->prev = node->prev;

    node->next = hdr->free;
    node->prev = 0;
    hdr->free = link;
}

void wst_listi_hdr_reset(WstListiHdr *hdr, size_t stride)
{
    assert(hdr != NULL);

    hdr->free = 0;

    if (hdr->len == 0)
        return;

    WstListiNodeHdr *sentinel = index_node_at(hdr, stride, 0);
    sentinel->next = 0;
    sentinel->prev = 0;
}

void wst_listi_hdr_add_free(WstListiHdr *hdr, size_t stride, uint32_t first,
                            uint32_t count)
{
    assert(hdr != NULL);

    if (count == 0)
        return;

    assert(first != 0);
    assert(first <= hdr->len);
    assert(count <= hdr->len - first + 1);

    for (uint32_t i = count; i > 0; i--) {
        uint32_t link = first + i - 1;
        WstListiNodeHdr *node = index_node_at(hdr, stride, link);
        node->next = hdr->free;
        node->prev = 0;
        hdr->free = link;
    }
}

void wst_listi_hdr_free(WstListiHdr *hdr)
{
    assert(hdr != NULL);

    free(hdr->nodes);
    hdr->nodes = NULL;
    hdr->cap = 0;
    hdr->len = 0;
    hdr->free = 0;
}

WstListiNodeHdr *wst_listi_hdr_find(const WstListiHdr *hdr, size_t stride,
                                    size_t data_off, const void *value,
                                    size_t value_size)
{
    assert(hdr != NULL);
    assert(value != NULL);
    assert(value_size > 0);

    if (hdr->len == 0)
        return NULL;

    uint32_t link = index_node_at(hdr, stride, 0)->next;
    while (link != 0) {
        WstListiNodeHdr *node = index_node_at(hdr, stride, link);
        const void *data = (const unsigned char *)node + data_off;
        if (memcmp(data, value, value_size) == 0)
            return node;
        link = node->next;
    }
    return NULL;
}

ssize_t wst_listi_hdr_find_index(const WstListiHdr *hdr, size_t stride,
                                 size_t data_off, const void *value,
                                 size_t value_size)
{
    assert(hdr != NULL);
    assert(value != NULL);
    assert(value_size > 0);

    if (hdr->len == 0)
        return -1;

    uint32_t link = index_node_at(hdr, stride, 0)->next;
    size_t pos = 0;
    while (link != 0) {
        WstListiNodeHdr *node = index_node_at(hdr, stride, link);
        const void *data = (const unsigned char *)node + data_off;
        if (memcmp(data, value, value_size) == 0)
            return (ssize_t)pos;
        link = node->next;
        pos++;
    }
    return -1;
}

WstListErr wst_listi_hdr_verify(const WstListiHdr *hdr, size_t stride)
{
    if (hdr == NULL)
        return WST_LIST_ERR_NULL;

    if (stride < sizeof(WstListiNodeHdr))
        return WST_LIST_ERR_CORRUPT;

    if (hdr->cap > UINT32_MAX)
        return WST_LIST_ERR_CORRUPT;

    if (hdr->len > hdr->cap)
        return WST_LIST_ERR_CORRUPT;

    if (hdr->cap == 0) {
        if (hdr->nodes != NULL || hdr->len != 0 || hdr->free != 0)
            return WST_LIST_ERR_CORRUPT;
        return WST_LIST_NO_ERR;
    }

    if (hdr->nodes == NULL || hdr->len == 0)
        return WST_LIST_ERR_CORRUPT;

    uint32_t head = index_node_at(hdr, stride, 0)->next;
    uint32_t tail = index_node_at(hdr, stride, 0)->prev;

    if (head > hdr->len || tail > hdr->len || hdr->free > hdr->len)
        return WST_LIST_ERR_CORRUPT;

    if ((head == 0) != (tail == 0))
        return WST_LIST_ERR_CORRUPT;

    if (head != 0) {
        if (index_node_at(hdr, stride, head)->prev != 0 ||
            index_node_at(hdr, stride, tail)->next != 0)
            return WST_LIST_ERR_CORRUPT;
    }

    if (hdr->free != 0 &&
        index_node_at(hdr, stride, hdr->free)->prev != 0)
        return WST_LIST_ERR_CORRUPT;

    if (head == 0 && hdr->free == 0)
        return WST_LIST_ERR_CORRUPT;

    return WST_LIST_NO_ERR;
}
