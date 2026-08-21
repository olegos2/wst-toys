#ifndef TOYS_DS_H
#define TOYS_DS_H

#include <stddef.h>
#include <stdlib.h>


/** Grown block header, lives right before the exposed array. */
typedef struct {
    size_t length;
    size_t cap;
} VecHeader;

/** Grows the vector to hold one more entry and bumps length.
 * Returns the new base, or NULL on allocation failure with the
 * vector left untouched. */
void *vec_reserve(void *arr, size_t entry_sz);

#define vec_header(v) ((VecHeader *)(v) - 1)
#define vec_base(h)   (void *)((VecHeader *)(h) + 1)
#define vec_len(v)    ((v) ? vec_header(v)->length : 0)
#define vec_cap(v)    ((v) ? vec_header(v)->cap : 0)

/**
 * Appends val to the vector, does nothing on allocation failure.
 * Entries are read and written with plain indexing, v[i].
 */
#define vec_push(v, val) \
    do { \
        void *vec_tmp = vec_reserve((v), sizeof(*(v))); \
        if (vec_tmp != NULL) { \
            (v) = vec_tmp; \
            (v)[vec_len(v) - 1] = (val); \
        } \
    } while (0)

#define vec_free(v) \
    do { \
        if (v) { \
            free(vec_header(v)); \
            (v) = NULL; \
        } \
    } while (0)

#endif /* TOYS_DS_H */
