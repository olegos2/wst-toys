#include "toys/sort.h"
#include "toys/poly.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static size_t wst_qsort_partition(
    char *base, char *temp, char *pivot,
    size_t count, size_t size, WstComparator compar)
{
    assert(base != NULL && temp != NULL && pivot != NULL);

    memcpy(pivot, base + size * (count / 2), size);

    char *left = base;
    char *right = base + (count - 1) * size;

    while (1) {
        while (compar(left, pivot) < 0)
            left += size;
        while (compar(pivot, right) < 0)
            right -= size;

        if (left >= right)
            return (size_t)(left - base) / size;
        
        memcpy(temp, right, size);
        memcpy(right, left, size);
        memcpy(left, temp, size);
        
        left += size;
        right -= size;
    }

    return 0;
}

static void wst_qsort_impl(
    char *base, char *temp, char *pivot,
    size_t count, size_t size, WstComparator compar)
{
    if (count < 2)
        return;

    size_t split_idx = wst_qsort_partition(base, temp, pivot, count, size, compar);

    wst_qsort_impl(base, temp, pivot, split_idx, size, compar);
    wst_qsort_impl(base + split_idx * size, temp, pivot, count - split_idx, size, compar);
}

void wst_qsort(void *base, size_t count, size_t size,
               WstComparator compar)
{

}
