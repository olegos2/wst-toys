
#define STK_IMPL
#include "toys/stk.h"


static const unsigned char stk_canary[STK_CANARY_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
};

static unsigned char *stk_tail(const WstStkVoid *stk, size_t elem_size)
{
    return (unsigned char *)stk->data + stk->cap * elem_size;
}

static void stk_write_canary(WstStkVoid *stk, size_t elem_size)
{
    memcpy(stk_tail(stk, elem_size), stk_canary, STK_CANARY_SIZE);
}

static unsigned long stk_hash_bytes(unsigned long hash, const void *data, size_t n)
{
    /* djb2 */
    const unsigned char *bytes = data;
    for (size_t i = 0; i < n; i++)
        hash = hash * 33 + bytes[i];
    return hash;
}

static unsigned long stk_compute_hash(const WstStkVoid *stk)
{
    unsigned long hash = 5381;
    hash = stk_hash_bytes(hash, &stk->data, sizeof(stk->data));
    hash = stk_hash_bytes(hash, &stk->length, sizeof(stk->length));
    hash = stk_hash_bytes(hash, &stk->cap, sizeof(stk->cap));
    return hash;
}

void wst_stk_void_seal(WstStkVoid *stk)
{
    stk->hash = stk_compute_hash(stk);
}

WstStkErr wst_stk_void_verify(const WstStkVoid *stk, size_t elem_size)
{
    assert(stk != NULL);
    assert(elem_size != 0);

    if (stk->length > stk->cap) {
        LOG_E("length %zu exceeds capacity %zu", stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    if (stk->data == NULL) {
        if (stk->cap != 0) {
            LOG_E("null buffer with capacity %zu", stk->cap);
            return WST_STK_ERR_CORRUPT;
        }
        if (stk->hash == 0)
            return WST_STK_NO_ERR; /* fresh zero state: a real seal is never zero */
        if (stk->hash != stk_compute_hash(stk)) {
            LOG_E("checksum mismatch on empty stack");
            return WST_STK_ERR_CORRUPT;
        }
        return WST_STK_NO_ERR;
    }

    if (memcmp(stk_tail(stk, elem_size), stk_canary, STK_CANARY_SIZE) != 0) {
        LOG_E("tail canary damaged (length %zu, capacity %zu)",
              stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    if (stk->hash != stk_compute_hash(stk)) {
        LOG_E("checksum mismatch (length %zu, capacity %zu)",
              stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    return WST_STK_NO_ERR;
}

WstStkErr wst_stk_void_free(WstStkVoid *stk, size_t elem_size)
{
    assert(stk != NULL);

    WstStkErr err = wst_stk_void_verify(stk, elem_size);
    free(stk->data);
    stk->data = NULL;
    stk->length = 0;
    stk->cap = 0;
    wst_stk_void_seal(stk);

    LOG_D("freed stack");

    return err;
}

WstStkErr wst_stk_void_init(WstStkVoid *stk, size_t elem_size, size_t count)
{
    assert(stk != NULL);

    WstStkErr err = wst_stk_void_free(stk, elem_size);
    if (err != WST_STK_NO_ERR)
        return err;

    err = wst_stk_void_reserve(stk, count, elem_size);
    if (err != WST_STK_NO_ERR)
        return err;

    LOG_D("initialized stack with capacity %zu", stk->cap);

    return WST_STK_NO_ERR;
}

WstStkErr wst_stk_void_reserve(WstStkVoid *stk, size_t count, size_t elem_size)
{
    assert(stk != NULL);
    assert(elem_size != 0);

    WstStkErr err = wst_stk_void_verify(stk, elem_size);
    if (err != WST_STK_NO_ERR)
        return err;

    if (count <= stk->cap)
        return WST_STK_NO_ERR;

    if (count > (SIZE_MAX - STK_CANARY_SIZE) / elem_size) {
        LOG_E("capacity %zu overflows size_t", count);
        return WST_STK_ERR_OVERFLOW;
    }

    void *data = realloc(stk->data, count * elem_size + STK_CANARY_SIZE);
    if (data == NULL) {
        LOG_E("cannot reserve %zu elements: %s", count, strerror(errno));
        return WST_STK_ERR_NOMEM;
    }

    stk->data = data;
    stk->cap = count;
    stk_write_canary(stk, elem_size);
    wst_stk_void_seal(stk);
    LOG_D("grew to capacity %zu", count);

    return WST_STK_NO_ERR;
}

const char *wst_stk_err_str(WstStkErr err)
{
    switch (err) {
    case WST_STK_NO_ERR:
        return "no error";
    case WST_STK_ERR_NOMEM:
        return "out of memory";
    case WST_STK_ERR_EMPTY:
        return "stack is empty";
    case WST_STK_ERR_CORRUPT:
        return "stack is corrupt";
    case WST_STK_ERR_OVERFLOW:
        return "capacity overflow";
    default:
        return "unknown error";
    }
}
