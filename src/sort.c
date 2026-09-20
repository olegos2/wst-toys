#include "toys/sort.h"
#include "toys/string.h"
#include "toys/debug.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#define BUCKET_SORT_LEN ((1 << 8) + 1)

/**
 * Sorts strings by char at position `pos` (using radix/bucket sort).
 * Does not work with wide chars.
 *
 * Works by:
 * - counting occurencies of every possible byte in strings,
 * - creating index table from this count buffer,
 * - using a buffer to store sorted result,
 * - inserting strings (pointers) into this buffer using table from before,
 * - copying pointers back.
 *
 * Recurses for every string subset where all strings have same char at position `pos`
 * (except '\0') to sort subsets using char at `pos + 1`.
 *
 * `buf` is passed down to avoid allocating it on stack at every depth level.
 *
 * @param[inout] strings Array of strings where `[start, end)` needs to be sorted.
 * @param[inout] buf Buffer that has at least `end - start` length.
 * @param[in] start Starting index inside array.
 * @param[in] end Ending index non-inclusive.
 * @param[in] pos Character index to look for in every string.
 */
static void wst_str_sort_rec(const unsigned char *strings[], const unsigned char *buf[],
                             size_t start, size_t end, size_t pos)
{
    assert(strings != NULL);
    assert(buf != NULL);

    if (end - start < 2)
        return;

    /* `count[i + 1]` corresponds to number of occurencies
     * of a char value `i` in strings at position `pos` */
    size_t count[BUCKET_SORT_LEN] = { 0 };

    for (size_t i = start; i < end; i++)
        count[strings[i][pos] + 1]++;

    /* Convert `count` into table of indices in result buffer */
    for (size_t i = 2; i < BUCKET_SORT_LEN; i++)
        count[i] += count[i - 1];

    for (size_t i = start; i < end; i++)
        buf[count[strings[i][pos]]++] = strings[i];

    for (size_t i = start; i < end; i++) {
        /* Revert indices back to beginning of every subset. */
        count[strings[i][pos]]--;

        strings[i] = buf[i - start];
    }

    /* count[0] stores number of strings that have `\0` at `pos` */
    /* recurse for every subset except these */
    for (size_t i = 1; i < BUCKET_SORT_LEN - 1; i++) {
        wst_str_sort_rec(strings, buf, start + count[i],
                     start + count[i + 1], pos + 1);
    }
}

void wst_str_sort(const char *strings[], size_t count)
{
    assert(count == 0 || strings != NULL);

    if (count < 2)
        return;

    const unsigned char **buf = calloc(count, sizeof(*buf));
    if (buf == NULL)
        return;

    wst_str_sort_rec((const unsigned char **)strings, buf, 0, count, 0);

    free(buf);
}


static size_t wst_qsort_part(
    char *base, size_t count, size_t size, WstComparator compar, int depth)
{
    char *left = base;
    char *right = base + (count - 1) * size;
    char *pivot = base + size * (count / 2);

    LOG_V("%*sdepth %d: n=%zu", depth * 2, "", depth, count);

    while (1) {
        while (compar(left, pivot) < 0)
            left += size;
        while (compar(pivot, right) < 0)
            right -= size;

        size_t left_idx = (size_t)(left - base) / size;
        size_t right_idx = (size_t)(right - base) / size;
        size_t pivot_idx = (size_t)(pivot - base) / size;

        LOG_V("%*s  scan: left=%zu, right=%zu, pivot=%zu",
              depth * 2, "", left_idx, right_idx, pivot_idx);

        if (left >= right) {
            LOG_V("%*s  split at %zu", depth * 2, "", count - left_idx);
            return left_idx;
        }

        LOG_V("%*s  swap: [%zu] <-> [%zu]", depth * 2, "", left_idx, right_idx);

        /* Keep track of pivot element */
        if (pivot == left)
            pivot = right;
        else if (pivot == right)
            pivot = left;

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
    char *base, size_t count, size_t size, WstComparator compar, int depth)
{
    if (count < 2) {
        LOG_V("%*sdepth %d: n=%zu, nothing to do", depth * 2, "", depth, count);
        return;
    }

    size_t split_idx = wst_qsort_part(base, count, size, compar, depth);

    wst_qsort_impl(base, split_idx, size, compar, depth + 1);
    wst_qsort_impl(base + split_idx * size, count - split_idx, size, compar, depth + 1);
}

void wst_qsort(void *base, size_t count, size_t size,
               WstComparator compar)
{
    if (count < 2 || size == 0)
        return;

    assert(base != NULL);
    assert(compar != NULL);

    wst_qsort_impl(base, count, size, compar, 0);
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
