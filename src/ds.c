#include "toys/ds.h"

#include <stdlib.h>


/* Total block size for a given capacity, header included. */
static size_t vec_real_size(size_t cap, size_t entry_sz)
{
    return cap * entry_sz + sizeof(VecHeader);
}

void *vec_reserve(void *arr, size_t entry_sz)
{
    VecHeader *hdr = (arr != NULL) ? vec_header(arr) : NULL;
    size_t new_len = 1;
    size_t new_cap = 0;

    if (hdr != NULL) {
        new_len = hdr->length + 1;
        new_cap = hdr->cap;
    }

    if (new_len > new_cap) {
        new_cap = (new_cap == 0) ? 1 : (new_cap * 2);
        size_t new_sz = vec_real_size(new_cap, entry_sz);

        hdr = realloc(hdr, new_sz);
        if (hdr == NULL)
            return NULL;

        arr = vec_base(hdr);
        hdr->cap = new_cap;
    }

    hdr->length = new_len;
    return arr;
}
