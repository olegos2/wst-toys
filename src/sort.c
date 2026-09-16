#include "toys/sort.h"
#include "toys/string.h"
#include "toys/debug.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static size_t wst_qsort_part(
    char *base, char *pivot,
    size_t count, size_t size, WstComparator compar, int depth)
{
    assert(base != NULL && pivot != NULL);

    size_t pivot_idx = count / 2;
    memcpy(pivot, base + size * pivot_idx, size);

    char *left = base;
    char *right = base + (count - 1) * size;

    LOG_V("%*sdepth %d: n=%zu, pivot at idx %zu", depth * 2, "", depth, count, pivot_idx);

    while (1) {
        while (compar(left, pivot) < 0)
            left += size;
        while (compar(pivot, right) < 0)
            right -= size;

        size_t left_idx = (size_t)(left - base) / size;
        size_t right_idx = (size_t)(right - base) / size;

        LOG_V("%*s  scan: left=%zu, right=%zu", depth * 2, "", left_idx, right_idx);

        if (left >= right) {
            LOG_V("%*s  split at %zu",
                  depth * 2, "", count - left_idx);
            return left_idx;
        }

        LOG_V("%*s  swap: [%zu] <-> [%zu]", depth * 2, "", left_idx, right_idx);
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
    size_t count, size_t size, WstComparator compar, int depth)
{
    if (count < 2) {
        LOG_V("%*sdepth %d: n=%zu, nothing to do", depth * 2, "", depth, count);
        return;
    }

    size_t split_idx = wst_qsort_part(base, pivot, count, size, compar, depth);

    wst_qsort_impl(base, pivot, split_idx, size, compar, depth + 1);
    wst_qsort_impl(base + split_idx * size, pivot, count - split_idx, size, compar, depth + 1);
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

    wst_qsort_impl(base, pivot, count, size, compar, 0);

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

    LOG_V("n=%zu, elem=%zu", count, size);
    for (size_t pass = 1; pass < count; pass++) {
        bool swapped = false;
        for (size_t i = 0; i < count - pass; i++) {
            char *data1 = base_c + i * size;
            char *data2 = base_c + (i + 1) * size;

            if (compar(data1, data2) > 0) {
                LOG_V("  pass %zu: swap [%zu] <-> [%zu]", pass, i, i + 1);
                wst_memswp(data1, data2, size);
                swapped = true;
            }
        }
        if (!swapped) {
            LOG_V("  pass %zu: already sorted, done early", pass);
            break;
        }
    }
}
