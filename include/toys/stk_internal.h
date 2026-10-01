#ifndef TOYS_STK_INTERNAL_H
#define TOYS_STK_INTERNAL_H

#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef TOYS_STK_H
#  error "Internal header must not be used directly"
#endif

/** Canary tail size in bytes, shared by all instantiations. */
#define STK_CANARY_SIZE 8

/** Initial capacity on first growth. */
#define STK_INIT_CAP 8


/**
 * Error that may occur while operating on a stack.
 * Should be always handled after every operation.
 */
typedef enum {
    WST_STK_NO_ERR = 0,
    /** Allocation failure, previous contents are kept. */
    WST_STK_ERR_NOMEM,
    /** Pop from an empty stack. */
    WST_STK_ERR_EMPTY,
    /** Tail canary, struct hash, or length/capacity invariant is damaged. */
    WST_STK_ERR_CORRUPT,
    /** Requested capacity does not fit into size_t. */
    WST_STK_ERR_OVERFLOW,
} WstStkErr;


/** Short human-readable description of a stack error. */
const char *wst_stk_err_str(WstStkErr err);


/** Layout-identical untyped stack for generic implementations. */
typedef struct {
    /** Buffer elements, followed by a `STK_CANARY_SIZE` byte canary tail. */
    void *data;
    /** Number of elements currently in stack. */
    size_t length;
    /**
     * Current capacity of stack in elements.
     * Does not count in space required to store canary bytes.
     */
    size_t cap;
    /** Integrity checksum over data/length/cap, resealed after every mutation. */
    unsigned long hash;
} WstStkVoid;

/** Check length/capacity invariants, canary tail and checksum. */
WstStkErr wst_stk_void_verify(const WstStkVoid *stk, size_t elem_size);

/** Free the buffer and zero the struct. Refuses on verification failure. */
WstStkErr wst_stk_void_free(WstStkVoid *stk, size_t elem_size);

/** Reset to empty and reserve `count` elements, reports prior damage. */
WstStkErr wst_stk_void_init(WstStkVoid *stk, size_t elem_size, size_t count);

/** Grow so the stack holds at least `count` elements. */
WstStkErr wst_stk_void_reserve(WstStkVoid *stk, size_t count, size_t elem_size);

/** Recompute the stored checksum after a mutation. */
void wst_stk_void_seal(WstStkVoid *stk);

#endif /* TOYS_STK_INTERNAL_H */


#if !defined(STK_T)
#  undef STK_ELEM
#  undef STK_T
#  undef STK_F
#  undef STK_FMT
#  undef STK_REF
#  define STK_ELEM double
#  define STK_T WstStkDouble
#  define STK_F(name) wst_stk_double_ ## name
#  define STK_FMT "%lg"
#endif

#if !defined(STK_ELEM)
#  error STK_ELEM must be defined with the name of stack element type
#endif

#if !defined(STK_F)
#  error STK_F must be defined as function macro that generates names for stack impl functions
#endif

#if !defined(STK_REF) && !defined(STK_FMT)
#  error STK_FMT must be defined with stack element format string, or define STK_REF
#endif


#define stk_free STK_F(free)
#define stk_init STK_F(init)
#define stk_reserve STK_F(reserve)
#define stk_push STK_F(push)
#define stk_pop STK_F(pop)
#define stk_verify STK_F(verify)
#define stk_prepare_push STK_F(prepare_push)


/** A stack with elements of fixed type. */
typedef struct {
    /** Elements of stack, followed by a `STK_CANARY_SIZE` byte canary tail. */
    STK_ELEM *data;
    /** Number of elements currently in stack. */
    size_t length;
    /**
     * Current capacity of stack in elements.
     * Does not count in space required to store canary bytes.
     */
    size_t cap;
    /** Integrity checksum over data/length/cap, resealed after every mutation. */
    unsigned long hash;
} STK_T;

/** Check length/capacity invariants, canary tail and checksum. */
WstStkErr stk_verify(const STK_T *stk);

/** Free the buffer and zero the struct. Refuses on verification failure. */
WstStkErr stk_free(STK_T *stk);

/** Reset to empty and reserve `count` elements, reports prior damage. */
WstStkErr stk_init(STK_T *stk, size_t count);

/** Grow so the stack holds at least `count` elements. */
WstStkErr stk_reserve(STK_T *stk, size_t count);

#ifdef STK_REF

/** Push a value to stack top by pointer, growing when needed. */
WstStkErr stk_push(STK_T *stk, const STK_ELEM *value);

#else /* !STK_REF */

/** Push a value to stack top, growing when needed. */
WstStkErr stk_push(STK_T *stk, STK_ELEM value);

#endif /* !STK_REF */

/** Pop value from stack top into `out`, or discard it when `out` is NULL. */
WstStkErr stk_pop(STK_T *stk, STK_ELEM *out);


#ifdef STK_IMPL

/* Typed and untyped layouts must stay identical, wrappers cast between them. */
static_assert(sizeof(STK_T) == sizeof(WstStkVoid), "stack layout mismatch");
static_assert(offsetof(STK_T, data) == offsetof(WstStkVoid, data), "data offset mismatch");
static_assert(offsetof(STK_T, length) == offsetof(WstStkVoid, length), "length offset mismatch");
static_assert(offsetof(STK_T, cap) == offsetof(WstStkVoid, cap), "cap offset mismatch");
static_assert(offsetof(STK_T, hash) == offsetof(WstStkVoid, hash), "hash offset mismatch");

WstStkErr stk_verify(const STK_T *stk)
{
    assert(stk != NULL);
    return wst_stk_void_verify((const WstStkVoid *)stk, sizeof(STK_ELEM));
}

WstStkErr stk_free(STK_T *stk)
{
    assert(stk != NULL);
    return wst_stk_void_free((WstStkVoid *)stk, sizeof(STK_ELEM));
}

WstStkErr stk_init(STK_T *stk, size_t count)
{
    assert(stk != NULL);
    return wst_stk_void_init((WstStkVoid *)stk, sizeof(STK_ELEM), count);
}

WstStkErr stk_reserve(STK_T *stk, size_t count)
{
    assert(stk != NULL);
    return wst_stk_void_reserve((WstStkVoid *)stk, count, sizeof(STK_ELEM));
}

/** Increase stack capacity by factor of 2, or make initial reservation. */
static WstStkErr stk_prepare_push(STK_T *stk)
{
    assert(stk != NULL);

    WstStkErr err = stk_verify(stk);
    if (err != WST_STK_NO_ERR || stk->length < stk->cap)
        return err;

    size_t limit = (SIZE_MAX - STK_CANARY_SIZE) / sizeof(STK_ELEM);
    size_t cap = (stk->cap == 0) ? STK_INIT_CAP : stk->cap * 2;
    if (cap < stk->cap || cap > limit)
        cap = limit;

    err = stk_reserve(stk, cap);
    if (err != WST_STK_NO_ERR)
        return err;

    if (stk->length == stk->cap) {
        LOG_E("cannot grow past %zu elements", stk->cap);
        return WST_STK_ERR_OVERFLOW;
    }
    return WST_STK_NO_ERR;
}

#ifdef STK_REF

WstStkErr stk_push(STK_T *stk, const STK_ELEM *value)
{
    assert(value != NULL);

    WstStkErr err = stk_prepare_push(stk);
    if (err != WST_STK_NO_ERR)
        return err;

    stk->data[stk->length++] = *value;
    wst_stk_void_seal((WstStkVoid *)stk);
    LOG_V("push [%p], length %zu", (const void *)value, stk->length);
    return WST_STK_NO_ERR;
}

#else /* !STK_REF */

WstStkErr stk_push(STK_T *stk, STK_ELEM value)
{
    WstStkErr err = stk_prepare_push(stk);
    if (err != WST_STK_NO_ERR)
        return err;

    stk->data[stk->length++] = value;
    wst_stk_void_seal((WstStkVoid *)stk);
    LOG_V("push " STK_FMT ", length %zu", value, stk->length);
    return WST_STK_NO_ERR;
}

#endif /* !STK_REF */


WstStkErr stk_pop(STK_T *stk, STK_ELEM *out)
{
    assert(stk != NULL);

    WstStkErr err = stk_verify(stk);
    if (err != WST_STK_NO_ERR)
        return err;

    if (stk->length == 0) {
        LOG_E("pop from an empty stack");
        return WST_STK_ERR_EMPTY;
    }

#ifdef STK_REF

    stk->length--;
    if (out != NULL)
        *out = stk->data[stk->length];
    wst_stk_void_seal((WstStkVoid *)stk);
    LOG_V("pop [%p], length %zu", (const void *)out, stk->length);

#else /* !STK_REF */

    STK_ELEM value = stk->data[--stk->length];
    if (out != NULL)
        *out = value;
    wst_stk_void_seal((WstStkVoid *)stk);
    LOG_V("pop " STK_FMT ", length %zu", value, stk->length);

#endif /* !STK_REF */

    return WST_STK_NO_ERR;
}

#endif /* STK_IMPL */

#undef stk_free
#undef stk_init
#undef stk_reserve
#undef stk_push
#undef stk_pop
#undef stk_verify
#undef stk_prepare_push

#undef STK_FMT
#undef STK_REF
#undef STK_F
#undef STK_T
#undef STK_ELEM
