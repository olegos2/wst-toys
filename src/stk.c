
#define STK_IMPL
#include "toys/stk.h"

#include <stdbool.h>


#if STK_USE_CANARY
/** Stack canary data bytes. */
static const unsigned char stk_canary[STK_CANARY_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
};
#endif

/** Get pointer to first byte beyond current stack effective capacity. */
static unsigned char *stk_tail(const WstStkVoid *stk, size_t elem_size)
{
    return (unsigned char *)stk->data + stk->cap * elem_size;
}

/** Write canary to stack tail. */
static void stk_write_canary(WstStkVoid *stk, size_t elem_size)
{
#if STK_USE_CANARY
    memcpy(stk_tail(stk, elem_size), stk_canary, STK_CANARY_SIZE);
#else
    (void)stk;
    (void)elem_size;
#endif
}

/** Continue djb2 hash with new data. */
static unsigned long stk_hash_bytes(unsigned long hash, const void *data, size_t n)
{
    /* djb2 */
    const unsigned char *bytes = data;
    for (size_t i = 0; i < n; i++)
        hash = hash * 33 + bytes[i];
    return hash;
}

/** Compute integrity checksum over the struct past the hash field. */
static unsigned long stk_compute_hash(const WstStkVoid *stk)
{
    unsigned long hash = 5381;
    const unsigned char *rest = (const unsigned char *)stk + sizeof(stk->hash);
    hash = stk_hash_bytes(hash, rest, sizeof(*stk) - sizeof(stk->hash));
    return hash;
}

void wst_stk_void_seal(WstStkVoid *stk)
{
    assert(stk != NULL);
#if STK_USE_HASH
    stk->hash = stk_compute_hash(stk);
#else
    (void)stk;
#endif
}

WstStkErr wst_stk_void_verify(const WstStkVoid *stk, size_t elem_size)
{
    if (stk == NULL || elem_size == 0) {
        LOG_E("cannot verify null stack");
        return WST_STK_ERR_NULL;
    }

    if (stk->length > stk->cap) {
        LOG_E("length %zu exceeds capacity %zu", stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }
    if (stk->cap > (SIZE_MAX - STK_TAIL_SIZE) / elem_size) {
        LOG_E("capacity %zu overflows size_t", stk->cap);
        return WST_STK_ERR_CORRUPT;
    }

    if (stk->data == NULL) {
        if (stk->cap != 0) {
            LOG_E("null buffer with capacity %zu", stk->cap);
            return WST_STK_ERR_CORRUPT;
        }

        if (stk->hash == 0)
            return WST_STK_NO_ERR; /* fresh zero state, a real seal is never zero */

#if STK_USE_HASH
        if (stk->hash != stk_compute_hash(stk)) {
            LOG_E("checksum mismatch on empty stack");
            return WST_STK_ERR_CORRUPT;
        }
#endif
        return WST_STK_NO_ERR;
    }

#if STK_USE_CANARY
    if (memcmp(stk_tail(stk, elem_size), stk_canary, STK_CANARY_SIZE) != 0) {
        LOG_E("tail canary damaged (length %zu, capacity %zu)",
              stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }
#endif

#if STK_USE_HASH
    if (stk->hash != stk_compute_hash(stk)) {
        LOG_E("checksum mismatch (length %zu, capacity %zu)",
              stk->length, stk->cap);
        return WST_STK_ERR_CORRUPT;
    }
#endif

    return WST_STK_NO_ERR;
}

WstStkErr wst_stk_void_free(WstStkVoid *stk, size_t elem_size)
{
    assert(stk != NULL);

    WstStkErr err = wst_stk_void_verify(stk, elem_size);
    if (err != WST_STK_NO_ERR)
        return err;

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

    if (count > (SIZE_MAX - STK_TAIL_SIZE) / elem_size) {
        LOG_E("capacity %zu overflows size_t", count);
        return WST_STK_ERR_OVERFLOW;
    }

    void *data = realloc(stk->data, count * elem_size + STK_TAIL_SIZE);
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

void wst_stk_void_dump(const WstStkVoid *stk, size_t elem_size, const char *st_name)
{
    if (stk == NULL || elem_size == 0) {
        fprintf(stderr, "stack (%s): <null>\n", st_name);
        return;
    }

    bool sane = stk->data != NULL &&
                stk->length <= stk->cap &&
                stk->cap <= (SIZE_MAX - STK_TAIL_SIZE) / elem_size;

    fprintf(stderr, "stack (%s) <%p>:\n"
            "  data:   <%p>\n"
            "  length: %zu\n"
            "  cap:    %zu\n",
            st_name, (const void *)stk, stk->data, stk->length, stk->cap);

#if STK_USE_HASH
    unsigned long computed = stk_compute_hash(stk);
    fprintf(stderr, "  checksum:\n"
            "    stored: 0x%lx\n"
            "    computed: 0x%lx\n"
            "    result: %s\n",
            stk->hash, computed, stk->hash == computed ? "OK" : "MISMATCH");
#else
    fprintf(stderr, "  checksum: NOT COMPILED\n");
#endif

#if STK_USE_CANARY
    fprintf(stderr, "  canary:\n    expected: ");
    for (size_t i = 0; i < STK_CANARY_SIZE; i++)
        fprintf(stderr, "%02X ", stk_canary[i]);
    fprintf(stderr, "\n");
    if (sane) {
        unsigned char *tail = stk_tail(stk, elem_size);
        bool match = memcmp(tail, stk_canary, STK_CANARY_SIZE) == 0;
        fprintf(stderr, "    actual: ");
        for (size_t i = 0; i < STK_CANARY_SIZE; i++)
            fprintf(stderr, "%02X ", tail[i]);
        fprintf(stderr, "\n    result: %s\n", match ? "OK" : "MISMATCH");
        if (!match) {
            fprintf(stderr, "    damaged at byte: ");
            for (size_t i = 0; i < STK_CANARY_SIZE; i++) {
                if (tail[i] != stk_canary[i])
                    fprintf(stderr, " %zu", i);
            }
            fprintf(stderr, "\n");
        }
    } else {
        fprintf(stderr, "    actual: omitted due to sanity check fail\n");
    }
#else
    fprintf(stderr, "  canary: NOT COMPILED\n");
#endif

    if (sane) {
        fprintf(stderr, "  buffer [%zu x %zu bytes]:\n", stk->cap, elem_size);
        unsigned char *bytes = stk->data;
        for (size_t i = 0; i < stk->cap; i++) {
            fprintf(stderr, "    [%4zu]:", i);
            for (size_t j = 0; j < elem_size; j++)
                fprintf(stderr, " %02X", bytes[i * elem_size + j]);
            fprintf(stderr, "\n");
        }
    } else {
        fprintf(stderr, "  buffer: omitted due to sanity check fail\n");
    }
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
    case WST_STK_ERR_NULL:
        return "null stack";
    default:
        return "unknown error";
    }
}
