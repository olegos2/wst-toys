#ifndef TOYS_VEC_INTERNAL_H
#define TOYS_VEC_INTERNAL_H

#include "toys/debug.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TOYS_VEC_H
#  error "Internal header must not be used directly"
#endif

/** Canary tail size in bytes, shared by all instantiations. */
#define VEC_CANARY_SIZE 8

/** Initial capacity on first growth. */
#define VEC_INIT_CAP 8

#if !defined(VEC_USE_CANARY)
#  ifdef WST_DEBUG
#    define VEC_USE_CANARY 1
#  else
#    define VEC_USE_CANARY 0
#  endif
#endif

#if !defined(VEC_USE_HASH)
#  ifdef WST_DEBUG
#    define VEC_USE_HASH 1
#  else
#    define VEC_USE_HASH 0
#  endif
#endif

#if !defined(VEC_USE_CONTENT_HASH)
#  ifdef WST_DEBUG
#    define VEC_USE_CONTENT_HASH 1
#  else
#    define VEC_USE_CONTENT_HASH 0
#  endif
#endif
/* Content hashing extends the struct checksum, inert without VEC_USE_HASH. */

#if VEC_USE_CANARY
#  define VEC_HEAD_SIZE VEC_CANARY_SIZE
#  define VEC_TAIL_SIZE VEC_CANARY_SIZE
#else
#  define VEC_HEAD_SIZE 0
#  define VEC_TAIL_SIZE 0
#endif

#define VEC_CANARY_TOTAL (VEC_HEAD_SIZE + VEC_TAIL_SIZE)

/** Reseal after mutation, compiled out when checksums are off. */
#if VEC_USE_HASH
#  define VEC_SEAL(vec, sz) wst_vec_void_seal((vec), (sz))
#else
#  define VEC_SEAL(vec, sz) do { (void)(vec); (void)(sz); } while (0)
#endif


/**
 * Error that may occur while operating on a vector.
 * Should be always handled after every operation.
 */
typedef enum {
    WST_VEC_NO_ERR = 0,
    /** Allocation failure, previous contents are kept. */
    WST_VEC_ERR_NOMEM,
    /** Pop from an empty vector. */
    WST_VEC_ERR_EMPTY,
    /** Null vector or zero element size passed for verification. */
    WST_VEC_ERR_NULL,
    /** Tail canary, struct hash, or length/capacity invariant is damaged. */
    WST_VEC_ERR_CORRUPT,
    /** Requested capacity does not fit into size_t. */
    WST_VEC_ERR_OVERFLOW,
} WstVecErr;


/** Short human-readable description of a vector error. */
const char *wst_vec_err_str(WstVecErr err);


/** Layout-identical untyped vector for generic implementations. */
typedef struct {
    /** Integrity checksum over the rest of the struct, resealed after every mutation. */
    unsigned long hash;
    /** Buffer elements, surrounded by canaries when VEC_USE_CANARY is set. */
    void *data;
    /** Number of elements currently in vector. */
    size_t length;
    /**
     * Current capacity of vector in elements.
     * Does not count in space required to store canary bytes.
     */
    size_t cap;
} WstVecVoid;

/** Check length/capacity invariants, canary tail and checksum. */
WstVecErr wst_vec_void_verify(const WstVecVoid *vec, size_t elem_size);

/** Free the buffer and zero the struct. Refuses on verification failure. */
WstVecErr wst_vec_void_free(WstVecVoid *vec, size_t elem_size);

/** Reset to empty and reserve `count` elements, reports prior damage. */
WstVecErr wst_vec_void_init(WstVecVoid *vec, size_t elem_size, size_t count);

/** Grow so the vector holds at least `count` elements. */
WstVecErr wst_vec_void_reserve(WstVecVoid *vec, size_t count, size_t elem_size);

/** Recompute the stored checksum after a mutation. */
void wst_vec_void_seal(WstVecVoid *vec, size_t elem_size);

/** Dump struct fields, checksum pair, canary and buffer contents to stderr. */
void wst_vec_void_dump(const WstVecVoid *vec, size_t elem_size, const char *vec_name);

#endif /* TOYS_VEC_INTERNAL_H */


#ifdef VEC_T
#if !defined(VEC_ELEM)
#  error VEC_ELEM must be defined with the name of vector element type
#endif

#if !defined(VEC_F)
#  error VEC_F must be defined as function macro that generates names for vector impl functions
#endif

#if !defined(VEC_REF) && !defined(VEC_FMT)
#  error VEC_FMT must be defined with vector element format string, or define VEC_REF
#endif


#define vec_free VEC_F(free)
#define vec_init VEC_F(init)
#define vec_reserve VEC_F(reserve)
#define vec_push VEC_F(push)
#define vec_pop VEC_F(pop)
#define vec_verify VEC_F(verify)
#define vec_prepare_push VEC_F(prepare_push)
#define vec_dump VEC_F(dump)


/** A vector with elements of fixed type. */
typedef struct {
    /** Integrity checksum over the rest of the struct, resealed after every mutation. */
    unsigned long hash;
    /** Buffer elements, surrounded by canaries when VEC_USE_CANARY is set. */
    VEC_ELEM *data;
    /** Number of elements currently in vector. */
    size_t length;
    /**
     * Current capacity of vector in elements.
     * Does not count in space required to store canary bytes.
     */
    size_t cap;
} VEC_T;

/** Check length/capacity invariants, canary tail and checksum. */
WstVecErr vec_verify(const VEC_T *vec);

/** Free the buffer and zero the struct. Refuses on verification failure. */
WstVecErr vec_free(VEC_T *vec);

/** Reset to empty and reserve `count` elements, reports prior damage. */
WstVecErr vec_init(VEC_T *vec, size_t count);

/** Grow so the vector holds at least `count` elements. */
WstVecErr vec_reserve(VEC_T *vec, size_t count);

#ifdef VEC_REF

/** Push a value to vector top by pointer, growing when needed. */
WstVecErr vec_push(VEC_T *vec, const VEC_ELEM *value);

#else /* !VEC_REF */

/** Push a value to vector top, growing when needed. */
WstVecErr vec_push(VEC_T *vec, VEC_ELEM value);

#endif /* !VEC_REF */

/** Pop value from vector top into `out`, or discard it when `out` is NULL. */
WstVecErr vec_pop(VEC_T *vec, VEC_ELEM *out);

#define vec_ensure VEC_F(ensure)
#define vec_set VEC_F(set)
#define vec_get VEC_F(get)
#define vec_resize VEC_F(resize)
#define vec_clear VEC_F(clear)
#define vec_seal VEC_F(seal)

/** Grow length to `n`, zero-filling the new part. Smaller requests are a no-op. */
WstVecErr vec_ensure(VEC_T *vec, size_t n);

#ifdef VEC_REF

/** Write `value` at index `i`, resealing afterwards. Index must be below length. */
WstVecErr vec_set(VEC_T *vec, size_t i, const VEC_ELEM *value);

/** Read value at index `i` into `out`. Index must be below length. */
void vec_get(const VEC_T *vec, size_t i, VEC_ELEM *out);

#else /* !VEC_REF */

/** Write `value` at index `i`, resealing afterwards. Index must be below length. */
WstVecErr vec_set(VEC_T *vec, size_t i, VEC_ELEM value);

/** Read value at index `i`. Index must be below length. */
VEC_ELEM vec_get(const VEC_T *vec, size_t i);

#endif /* !VEC_REF */

/** Set length to `n`, zero-filling growth. */
WstVecErr vec_resize(VEC_T *vec, size_t n);

/** Set length to zero, keeping capacity. */
WstVecErr vec_clear(VEC_T *vec);

/** Recompute the stored checksum after direct buffer work. */
void vec_seal(VEC_T *vec);

/** Dump vector contents, canary and checksum state to stderr. */
void vec_dump(const VEC_T *vec);


#ifdef VEC_IMPL

/* Typed and untyped layouts must stay identical, wrappers cast between them. */
static_assert(sizeof(VEC_T) == sizeof(WstVecVoid), "vector layout mismatch");
static_assert(offsetof(VEC_T, data) == offsetof(WstVecVoid, data), "data offset mismatch");
static_assert(offsetof(VEC_T, length) == offsetof(WstVecVoid, length), "length offset mismatch");
static_assert(offsetof(VEC_T, cap) == offsetof(WstVecVoid, cap), "cap offset mismatch");
static_assert(offsetof(VEC_T, hash) == offsetof(WstVecVoid, hash), "hash offset mismatch");

WstVecErr vec_verify(const VEC_T *vec)
{
    assert(vec != NULL);
    return wst_vec_void_verify((const WstVecVoid *)vec, sizeof(VEC_ELEM));
}

WstVecErr vec_free(VEC_T *vec)
{
    assert(vec != NULL);
    return wst_vec_void_free((WstVecVoid *)vec, sizeof(VEC_ELEM));
}

WstVecErr vec_init(VEC_T *vec, size_t count)
{
    assert(vec != NULL);
    return wst_vec_void_init((WstVecVoid *)vec, sizeof(VEC_ELEM), count);
}

WstVecErr vec_reserve(VEC_T *vec, size_t count)
{
    assert(vec != NULL);
    return wst_vec_void_reserve((WstVecVoid *)vec, count, sizeof(VEC_ELEM));
}

/** Increase vector capacity by factor of 2, or make initial reservation. */
static WstVecErr vec_prepare_push(VEC_T *vec)
{
    assert(vec != NULL);

    WstVecErr err = vec_verify(vec);
    if (err != WST_VEC_NO_ERR || vec->length < vec->cap)
        return err;

    size_t limit = (SIZE_MAX - VEC_CANARY_TOTAL) / sizeof(VEC_ELEM);
    size_t cap = (vec->cap == 0) ? VEC_INIT_CAP : vec->cap * 2;
    if (cap < vec->cap || cap > limit)
        cap = limit;

    err = vec_reserve(vec, cap);
    if (err != WST_VEC_NO_ERR)
        return err;

    if (vec->length == vec->cap) {
        LOG_E("cannot grow past %zu elements", vec->cap);
        return WST_VEC_ERR_OVERFLOW;
    }
    return WST_VEC_NO_ERR;
}

#ifdef VEC_REF

WstVecErr vec_push(VEC_T *vec, const VEC_ELEM *value)
{
    assert(value != NULL);

    WstVecErr err = vec_prepare_push(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    vec->data[vec->length++] = *value;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    LOG_V("push [%p], length %zu", (const void *)value, vec->length);
    return WST_VEC_NO_ERR;
}

#else /* !VEC_REF */

WstVecErr vec_push(VEC_T *vec, VEC_ELEM value)
{
    WstVecErr err = vec_prepare_push(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    vec->data[vec->length++] = value;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    LOG_V("push " VEC_FMT ", length %zu", value, vec->length);
    return WST_VEC_NO_ERR;
}

#endif /* !VEC_REF */


WstVecErr vec_pop(VEC_T *vec, VEC_ELEM *out)
{
    assert(vec != NULL);

    WstVecErr err = vec_verify(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    if (vec->length == 0) {
        LOG_E("pop from an empty vector");
        return WST_VEC_ERR_EMPTY;
    }

#ifdef VEC_REF

    vec->length--;
    if (out != NULL)
        *out = vec->data[vec->length];
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    LOG_V("pop [%p], length %zu", (const void *)out, vec->length);

#else /* !VEC_REF */

    VEC_ELEM value = vec->data[--vec->length];
    if (out != NULL)
        *out = value;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    LOG_V("pop " VEC_FMT ", length %zu", value, vec->length);

#endif /* !VEC_REF */

    return WST_VEC_NO_ERR;
}

WstVecErr vec_ensure(VEC_T *vec, size_t n)
{
    assert(vec != NULL);

    WstVecErr err = vec_reserve(vec, n);
    if (err != WST_VEC_NO_ERR)
        return err;

    if (n <= vec->length)
        return WST_VEC_NO_ERR;

    memset(vec->data + vec->length, 0, n - vec->length);
    vec->length = n;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    return WST_VEC_NO_ERR;
}

#ifdef VEC_REF

WstVecErr vec_set(VEC_T *vec, size_t i, const VEC_ELEM *value)
{
    assert(vec != NULL);
    assert(value != NULL);

    WstVecErr err = vec_verify(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    assert(i < vec->length);
    vec->data[i] = *value;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    return WST_VEC_NO_ERR;
}

void vec_get(const VEC_T *vec, size_t i, VEC_ELEM *out)
{
    assert(vec != NULL);
    assert(out != NULL);
    assert(i < vec->length);
    *out = vec->data[i];
}

#else /* !VEC_REF */

WstVecErr vec_set(VEC_T *vec, size_t i, VEC_ELEM value)
{
    assert(vec != NULL);

    WstVecErr err = vec_verify(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    assert(i < vec->length);
    vec->data[i] = value;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    return WST_VEC_NO_ERR;
}

VEC_ELEM vec_get(const VEC_T *vec, size_t i)
{
    assert(vec != NULL);
    assert(i < vec->length);
    return vec->data[i];
}

#endif /* !VEC_REF */

WstVecErr vec_resize(VEC_T *vec, size_t n)
{
    assert(vec != NULL);

    WstVecErr err = vec_reserve(vec, n);
    if (err != WST_VEC_NO_ERR)
        return err;

    if (n > vec->length)
        memset(vec->data + vec->length, 0, n - vec->length);
    vec->length = n;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    return WST_VEC_NO_ERR;
}

WstVecErr vec_clear(VEC_T *vec)
{
    assert(vec != NULL);

    WstVecErr err = vec_verify(vec);
    if (err != WST_VEC_NO_ERR)
        return err;

    vec->length = 0;
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
    return WST_VEC_NO_ERR;
}

void vec_seal(VEC_T *vec)
{
    assert(vec != NULL);
    VEC_SEAL((WstVecVoid *)vec, sizeof(VEC_ELEM));
}

void vec_dump(const VEC_T *vec)
{
    LOG_V("Dumping vector at %p", vec);

#define VEC_T_STR_1(type) #type
#define VEC_T_STR_2(type) VEC_T_STR_1(type)
    wst_vec_void_dump((const WstVecVoid *)vec, sizeof(VEC_ELEM),
                      VEC_T_STR_2(VEC_T));
#undef VEC_T_STR_1
#undef VEC_T_STR_2

#ifdef VEC_FMT
    if (vec != NULL &&
        vec->data != NULL &&
        vec->length <= vec->cap &&
        vec->cap <= (SIZE_MAX - VEC_TAIL_SIZE) / sizeof(VEC_ELEM))
    {
        fprintf(stderr, "  formatted repr:\n");
        for (size_t i = 0; i < vec->length; i++)
            fprintf(stderr, "    [%4zu]: " VEC_FMT "\n", i, vec->data[i]);
    }
    else
        fprintf(stderr, "  <elements omitted due to sanity check fail>\n");
#endif
}

#endif /* VEC_IMPL */

#undef vec_free
#undef vec_init
#undef vec_reserve
#undef vec_push
#undef vec_pop
#undef vec_verify
#undef vec_prepare_push
#undef vec_ensure
#undef vec_set
#undef vec_get
#undef vec_resize
#undef vec_clear
#undef vec_seal
#undef vec_dump

#undef VEC_FMT
#undef VEC_REF
#undef VEC_F
#undef VEC_T
#undef VEC_ELEM
#endif /* VEC_T */
