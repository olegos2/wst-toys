#include "toys/sort.h"
#include "toys/string.h"
#include "toys/debug.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static size_t wst_qsort_partition(
    char *base, char *pivot,
    size_t count, size_t size, WstComparator compar)
{
    assert(base != NULL && pivot != NULL);

    memcpy(pivot, base + size * (count / 2), size);

    char *left = base;
    char *right = base + (count - 1) * size;

    while (1) {
        while (compar(left, pivot) < 0)
            left += size;
        while (compar(pivot, right) < 0)
            right -= size;

        LOG_V("left = %zu (%p), right = %zu (%p)",
              (size_t)(left - base) / size, left,
              (size_t)(right - base) / size, right);

        if (left >= right) {
            LOG_V("Done partitioning");
            return (size_t)(left - base) / size;
        }
        
        wst_memswp(left, right, size);
        // memcpy(temp, right, size);
        // memcpy(right, left, size);
        // memcpy(left, temp, size);
        
        left += size;
        right -= size;
    }

    return 0;
}

static void wst_qsort_impl(
    char *base, char *pivot,
    size_t count, size_t size, WstComparator compar)
{
    if (count < 2) {
        LOG_V("count < 2, returning");
        return;
    }
    LOG_V("base = %p, pivot = %p, count = %zu, size = %zu, compar = %p",
          base, pivot, count, size, compar);

    size_t split_idx = wst_qsort_partition(base, pivot, count, size, compar);

    wst_qsort_impl(base, pivot, split_idx, size, compar);
    wst_qsort_impl(base + split_idx * size, pivot, count - split_idx, size, compar);
}

void wst_qsort(void *base, size_t count, size_t size,
               WstComparator compar)
{
    if (count < 2 || size == 0)
        return;

    assert(base != NULL);
    assert(compar != NULL);

    char *pivot = malloc(size);
    if (pivot == NULL)
        return;

    wst_qsort_impl(base, pivot, count, size, compar);

    free(pivot);
}

void wst_bsort(void *base, size_t count, size_t size,
               WstComparator compar)
{
    if (count < 2 || size == 0)
        return;

    assert(base != NULL);
    assert(compar != NULL);

    char *base_c = (char *)base;

    for (size_t pass = 1; pass < count; pass++) {
        for (size_t i = 0; i < count - pass; i++) {
            char *data1 = base_c + i * size;
            char *data2 = base_c + (i + 1) * size;

            if (compar(data1, data2) > 0) {
                LOG_V("Swapping indices %zu <-> %zu", i, i + 1);
                wst_memswp(data1, data2, size);
            }
        }
    }
}
