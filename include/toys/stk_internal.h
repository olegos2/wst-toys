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
# error "Internal header must not be used directly"
#endif

/** Canary tail size in bytes, shared by all instantiations. */
#define STK_CANARY_SIZE 8

/** Initial capacity on first growth. */
#define STK_INIT_CAP 8

#endif /* TOYS_STK_INTERNAL_H */

#if (!defined(STK_ELEM) || !defined(STK_T) || !defined(STK_F) || !defined(STK_FMT))
#  undef STK_ELEM
#  undef STK_T
#  undef STK_F
#  undef STK_FMT
#  define STK_ELEM double
#  define STK_T WstStkDouble
#  define STK_F(name) wst_stk_double_ ## name
#  define STK_FMT "%lg"
#endif


#define stk_canary STK_F(canary)

#define stk_free STK_F(free)
#define stk_init STK_F(init)
#define stk_reserve STK_F(reserve)
#define stk_push STK_F(push)
#define stk_pop STK_F(pop)
#define stk_verify STK_F(verify)

#define stk_tail STK_F(tail)
#define stk_write_canary STK_F(write_canary)



/** A stack with elements of fixed type. */
typedef struct {
    /** Elements of stack, followed by a STK_CANARY_SIZE byte canary tail. */
    STK_ELEM *data;
    /** Number of elements currently in stack. */
    size_t length;
    /**
     * Current capacity of stack in elements.
     * Does not count in space required to store canary bytes.
     */
    size_t cap;
} STK_T;


/** Free the stack buffer and zero out length/capacity. */
WstStkErr stk_free(STK_T *stk);

/** Drop any previous data, reset the stack to empty. */
WstStkErr stk_init(STK_T *stk);

/**
 * Grow the stack so it holds at least `count` elements in total.
 * Smaller requests are a no-op. Initializes the stack if needed.
 */
WstStkErr stk_reserve(STK_T *stk, size_t count);

/** Push a value on top, growing the stack as needed. */
WstStkErr stk_push(STK_T *stk, STK_ELEM value);

/** Pop the top value into `out`, or discard it when `out` is NULL. */
WstStkErr stk_pop(STK_T *stk, STK_ELEM *out);

/** Check length/capacity invariants and the canary tail. */
WstStkErr stk_verify(const STK_T *stk);

#ifdef STK_IMPL

static const unsigned char stk_canary[STK_CANARY_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
};

static unsigned char *stk_tail(const STK_T *stk)
{
    return (unsigned char *)stk->data + stk->cap * sizeof(STK_ELEM);
}

static void stk_write_canary(STK_T *stk)
{
    memcpy(stk_tail(stk), stk_canary, STK_CANARY_SIZE);
}

WstStkErr stk_verify(const STK_T *stk)
{
    assert(stk != NULL);

    if (stk->length > stk->cap) {
        LOG_E("length %zu exceeds capacity %zu", stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    if (stk->data == NULL) {
        if (stk->cap != 0) {
            LOG_E("null buffer with capacity %zu", stk->cap);
            return WST_STK_ERR_CORRUPT;
        }
        return WST_STK_NO_ERR;
    }

    if (memcmp(stk_tail(stk), stk_canary, STK_CANARY_SIZE) != 0) {
        LOG_E("tail canary damaged (length %zu, capacity %zu)",
              stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    return WST_STK_NO_ERR;
}

WstStkErr stk_free(STK_T *stk)
{
    assert(stk != NULL);

    WstStkErr err = stk_verify(stk);
    /* Should immediately return error here? */
    // if (err != WST_STK_NO_ERR)
    //     return err;

    free(stk->data);
    stk->data = NULL;
    stk->length = 0;
    stk->cap = 0;

    LOG_D("freed stack");

    return err;
}

WstStkErr stk_init(STK_T *stk)
{
    assert(stk != NULL);

    WstStkErr err = stk_free(stk);
    LOG_D("initialized stack");

    return err;
}

WstStkErr stk_reserve(STK_T *stk, size_t count)
{
    assert(stk != NULL);

    WstStkErr err = stk_verify(stk);
    if (err != WST_STK_NO_ERR)
        return err;

    if (count <= stk->cap)
        return WST_STK_NO_ERR;

    if (count > (SIZE_MAX - STK_CANARY_SIZE) / sizeof(STK_ELEM)) {
        LOG_E("capacity %zu overflows size_t", count);
        return WST_STK_ERR_OVERFLOW;
    }

    STK_ELEM *data = realloc(stk->data, count * sizeof(STK_ELEM) + STK_CANARY_SIZE);
    if (data == NULL) {
        LOG_E("cannot reserve %zu elements: %s", count, strerror(errno));
        return WST_STK_ERR_NOMEM;
    }

    stk->data = data;
    stk->cap = count;
    stk_write_canary(stk);
    LOG_D("grew to capacity %zu", count);

    return WST_STK_NO_ERR;
}

WstStkErr stk_push(STK_T *stk, STK_ELEM value)
{
    assert(stk != NULL);

    WstStkErr err = stk_verify(stk);
    if (err != WST_STK_NO_ERR)
        return err;

    if (stk->length == stk->cap) {
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
    }

    stk->data[stk->length++] = value;
    LOG_V("push " STK_FMT ", length %zu", value, stk->length);
    return WST_STK_NO_ERR;
}

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

    STK_ELEM value = stk->data[--stk->length];
    if (out != NULL)
        *out = value;
    LOG_V("pop " STK_FMT ", length %zu", value, stk->length);
    return WST_STK_NO_ERR;
}

#endif /* STK_IMPL */

#undef stk_canary
#undef stk_free
#undef stk_init
#undef stk_reserve
#undef stk_push
#undef stk_pop
#undef stk_verify
#undef stk_tail
#undef stk_write_canary

#undef STK_FMT
#undef STK_F
#undef STK_T
#undef STK_ELEM
