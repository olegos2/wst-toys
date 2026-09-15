#ifndef TOYS_SORT_H
#define TOYS_SORT_H

#include <stddef.h>

/**
 * A function that compares two objects pointed by params.
 * Should return integer `< 0` when `a < b`, `0` when `a == b`, `> 0` when `a > b`.
 */
typedef int (*WstComparator)(const void *a, const void *b);

/**
 * Sort array using quick sort.
 *
 * @param[in] base Base address of array.
 * @param[in] count Number of elements in array.
 * @param[in] size Size of a single array element.
 * @param[in] compar Comparator.
 */
void wst_qsort(void *base, size_t count, size_t size,
               WstComparator compar);

#endif /* TOYS_SORT_H */