
#define VEC_IMPL
#include "toys/vec.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>


#if VEC_USE_CANARY
/** Vector canary data bytes, front and tail. */
static const unsigned char vec_canary_front[VEC_CANARY_SIZE] = {
    0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xC0, 0xDE,
};
static const unsigned char vec_canary_tail[VEC_CANARY_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
};
#endif

/** Real allocation base, front canary included, `NULL` if `vec->data` is `NULL`. */
static unsigned char *vec_base(const WstVecVoid *vec)
{
    if (vec->data == NULL)
        return NULL;
    return (unsigned char *)vec->data - VEC_HEAD_SIZE;
}

/** Get pointer to first byte after current vector effective capacity. */
static unsigned char *vec_tail(const WstVecVoid *vec, size_t elem_size)
{
    return (unsigned char *)vec->data + vec->cap * elem_size;
}

/** Write canaries around vector elements. */
static void vec_write_canaries(WstVecVoid *vec, size_t elem_size)
{
#if VEC_USE_CANARY
    memcpy(vec_base(vec), vec_canary_front, VEC_CANARY_SIZE);
    memcpy(vec_tail(vec, elem_size), vec_canary_tail, VEC_CANARY_SIZE);
#else
    (void)vec;
    (void)elem_size;
#endif
}

/** Continue djb2 hash with new data. */
static unsigned long vec_hash_bytes(unsigned long hash, const void *data, size_t n)
{
    const unsigned char *bytes = data;
    for (size_t i = 0; i < n; i++)
        hash = hash * 33 + bytes[i];
    return hash;
}

/** Compute integrity checksum over the struct past the hash field. */
static unsigned long vec_compute_hash(const WstVecVoid *vec, size_t elem_size)
{
    unsigned long hash = 5381;
    const unsigned char *rest = (const unsigned char *)vec + sizeof(vec->hash);
    hash = vec_hash_bytes(hash, rest, sizeof(*vec) - sizeof(vec->hash));

#if VEC_USE_CONTENT_HASH
    const unsigned char *elems = vec->data;
    for (size_t i = 0; i < vec->length; i++)
        hash = vec_hash_bytes(hash, elems + i * elem_size, elem_size);
#else
    (void)elem_size;
#endif

    return hash;
}

void wst_vec_void_seal(WstVecVoid *vec, size_t elem_size)
{
    assert(vec != NULL);
#if VEC_USE_HASH
    vec->hash = vec_compute_hash(vec, elem_size);
#else
    (void)vec;
    (void)elem_size;
#endif
}

WstVecErr wst_vec_void_verify(const WstVecVoid *vec, size_t elem_size)
{
    if (vec == NULL || elem_size == 0) {
        LOG_E("cannot verify null vector");
        return WST_VEC_ERR_NULL;
    }

    if (vec->length > vec->cap) {
        LOG_E("length %zu exceeds capacity %zu", vec->length, vec->cap);
        return WST_VEC_ERR_CORRUPT;
    }

    if (vec->cap > (SIZE_MAX - VEC_CANARY_TOTAL) / elem_size) {
        LOG_E("capacity %zu overflows size_t", vec->cap);
        return WST_VEC_ERR_CORRUPT;
    }

    if (vec->data == NULL) {
        if (vec->cap != 0) {
            LOG_E("null buffer with capacity %zu", vec->cap);
            return WST_VEC_ERR_CORRUPT;
        }

        if (vec->hash == 0)
            return WST_VEC_NO_ERR; /* fresh zero state, a real seal is never zero */

#if VEC_USE_HASH
        if (vec->hash != vec_compute_hash(vec, elem_size)) {
            LOG_E("checksum mismatch on empty vector");
            return WST_VEC_ERR_CORRUPT;
        }
#endif
        return WST_VEC_NO_ERR;
    }

#if VEC_USE_CANARY
    if (memcmp(vec_base(vec), vec_canary_front, VEC_CANARY_SIZE) != 0) {
        LOG_E("front canary damaged (length %zu, capacity %zu)",
              vec->length, vec->cap);
        return WST_VEC_ERR_CORRUPT;
    }
    if (memcmp(vec_tail(vec, elem_size), vec_canary_tail, VEC_CANARY_SIZE) != 0) {
        LOG_E("tail canary damaged (length %zu, capacity %zu)",
              vec->length, vec->cap);
        return WST_VEC_ERR_CORRUPT;
    }
#endif

#if VEC_USE_HASH
    if (vec->hash != vec_compute_hash(vec, elem_size)) {
        LOG_E("checksum mismatch (length %zu, capacity %zu)",
              vec->length, vec->cap);
        return WST_VEC_ERR_CORRUPT;
    }
#endif

    return WST_VEC_NO_ERR;
}

WstVecErr wst_vec_void_free(WstVecVoid *vec, size_t elem_size)
{
    assert(vec != NULL);

    WstVecErr err = wst_vec_void_verify(vec, elem_size);
    if (err != WST_VEC_NO_ERR)
        return err;

    free(vec_base(vec));
    vec->data = NULL;
    vec->length = 0;
    vec->cap = 0;
    wst_vec_void_seal(vec, elem_size);

    LOG_D("freed vector");

    return err;
}

WstVecErr wst_vec_void_init(WstVecVoid *vec, size_t elem_size, size_t count)
{
    assert(vec != NULL);

    WstVecErr err = wst_vec_void_free(vec, elem_size);
    if (err != WST_VEC_NO_ERR)
        return err;

    err = wst_vec_void_reserve(vec, count, elem_size);
    if (err != WST_VEC_NO_ERR)
        return err;

    LOG_D("initialized vector with capacity %zu", vec->cap);

    return WST_VEC_NO_ERR;
}

WstVecErr wst_vec_void_reserve(WstVecVoid *vec, size_t count, size_t elem_size)
{
    assert(vec != NULL);
    assert(elem_size != 0);

    WstVecErr err = wst_vec_void_verify(vec, elem_size);
    if (err != WST_VEC_NO_ERR)
        return err;

    if (count <= vec->cap)
        return WST_VEC_NO_ERR;

    if (count > (SIZE_MAX - VEC_CANARY_TOTAL) / elem_size) {
        LOG_E("capacity %zu overflows size_t", count);
        return WST_VEC_ERR_OVERFLOW;
    }

    void *base = realloc(vec_base(vec),
                         count * elem_size + VEC_HEAD_SIZE + VEC_TAIL_SIZE);
    if (base == NULL) {
        LOG_E("cannot reserve %zu elements: %s", count, strerror(errno));
        return WST_VEC_ERR_NOMEM;
    }

    vec->data = (unsigned char *)base + VEC_HEAD_SIZE;
    vec->cap = count;
    vec_write_canaries(vec, elem_size);
    wst_vec_void_seal(vec, elem_size);
    LOG_D("grew to capacity %zu", count);

    return WST_VEC_NO_ERR;
}

static void wst_vec_void_dump_canary(const unsigned char *base, const char *name, bool sane,
                                     const unsigned char canary_exp[])
{
#if VEC_USE_CANARY
    fprintf(stderr, "  %s canary:\n    expected: ", name);
    for (size_t i = 0; i < VEC_CANARY_SIZE; i++)
        fprintf(stderr, "%02X ", canary_exp[i]);
    fprintf(stderr, "\n");

    if (!sane) {
        fprintf(stderr, "    actual: omitted due to sanity check fail\n");
        return;
    }

    bool match = memcmp(base, canary_exp, VEC_CANARY_SIZE) == 0;

    fprintf(stderr, "    actual:   ");
    for (size_t i = 0; i < VEC_CANARY_SIZE; i++)
        fprintf(stderr, "%02X ", base[i]);

    fprintf(stderr, "\n    result: %s\n", match ? "OK" : "MISMATCH");

    if (match)
        return;

    fprintf(stderr, "    damaged at byte: ");
    for (size_t i = 0; i < VEC_CANARY_SIZE; i++) {
        if (base[i] != canary_exp[i])
            fprintf(stderr, " %zu", i);
    }
    fprintf(stderr, "\n");

#else /* !VEC_USE_CANARY */
    fprintf(stderr, "  %s canary: disabled in build\n", name);
#endif /* !VEC_USE_CANARY */
}

void wst_vec_void_dump(const WstVecVoid *vec, size_t elem_size, const char *vec_name)
{
    if (vec == NULL || elem_size == 0) {
        fprintf(stderr, "vector (%s): <null>\n", vec_name);
        return;
    }

    bool sane = vec->data != NULL &&
                vec->length <= vec->cap &&
                vec->cap <= (SIZE_MAX - VEC_CANARY_TOTAL) / elem_size;

    fprintf(stderr, "vector (%s) <%p>:\n"
            "  data:   <%p>\n"
            "  length: %zu\n"
            "  cap:    %zu\n",
            vec_name, (const void *)vec, vec->data, vec->length, vec->cap);

#if VEC_USE_HASH
    if (sane) {
        unsigned long computed = vec_compute_hash(vec, elem_size);
        fprintf(stderr, "  checksum:\n"
                "    stored:   0x%016lx\n"
                "    computed: 0x%016lx\n"
                "    result: %s\n",
                vec->hash, computed, vec->hash == computed ? "OK" : "MISMATCH");
    } else {
        fprintf(stderr, "  checksum:\n"
                "    stored:   0x%016lx\n"
                "    computed: <omitted>\n",
                vec->hash);
    }
#else
    fprintf(stderr, "  checksum: disabled in build\n");
#endif

    wst_vec_void_dump_canary(vec_base(vec), "front",
                             sane, vec_canary_front);
    wst_vec_void_dump_canary(vec_tail(vec, elem_size), "tail",
                             sane, vec_canary_tail);

    if (sane) {
        fprintf(stderr, "  buffer [%zu x %zu bytes]:\n", vec->cap, elem_size);

        unsigned char *bytes = vec->data;
        for (size_t i = 0; i < vec->cap; i++) {
            fprintf(stderr, "    [%4zu]:", i);
            for (size_t j = 0; j < elem_size; j++)
                fprintf(stderr, " %02X", bytes[i * elem_size + j]);
            fprintf(stderr, "\n");
        }
    } else {
        fprintf(stderr, "  buffer: omitted due to sanity check fail\n");
    }
}

const char *wst_vec_err_str(WstVecErr err)
{
    switch (err) {
    case WST_VEC_NO_ERR:
        return "no error";
    case WST_VEC_ERR_NOMEM:
        return "out of memory";
    case WST_VEC_ERR_EMPTY:
        return "vector is empty";
    case WST_VEC_ERR_CORRUPT:
        return "vector is corrupt";
    case WST_VEC_ERR_OVERFLOW:
        return "capacity overflow";
    case WST_VEC_ERR_NULL:
        return "null vector";
    default:
        return "unknown error";
    }
}
