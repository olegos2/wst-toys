#ifndef TOYS_DS_H
#define TOYS_DS_H

#include "toys/debug.h"

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t length;
    size_t cap;
} VecHeader;

#define vec_header(v) ((VecHeader *)(v) - 1)
#define vec_base(h) (void *)(h + 1)
#define vec_len(v) ((v) ? vec_header(v)->length : 0)
#define vec_cap(v) ((v) ? vec_header(v)->capacity : 0)
// #define vec_put(vec, val)

static inline size_t vec_real_size(size_t cap, size_t entry_sz)
{
    return cap * entry_sz + sizeof(VecHeader);
}

static void *vec_reserve(void *arr, size_t entry_sz)
{
    VecHeader *hdr = (arr != NULL) ? vec_header(arr) : NULL;
    size_t new_len = 0, new_cap = 0;
    if (hdr != NULL) {
        new_len = hdr->length + 1;
        new_cap = hdr->cap;
    }

    if (new_len > new_cap) {
        new_cap = (new_cap == 0) ? 1 : (new_cap * 2);
        size_t new_sz = vec_real_size(new_cap, entry_sz);
    
        hdr = realloc(hdr, new_sz);
        if (hdr == NULL) return NULL;

        arr = vec_base(hdr);
        hdr->cap = new_cap;
    }

    hdr->length = new_len;
    return arr;
}

#define vec_put(v, entry) (vec_reserve(v, sizeof(entry)))

static bool vec_put(void **arr, size_t entry_sz, void *val)
{
    void *new_arr = vec_reserve(arr, entry_sz);
    if (new_arr == NULL) {
        LOG_E("Not enough mem for new cap");
        return NULL;
    }
}

#endif /* TOYS_DS_H */
