#include "tests_common.h"

#include "toys/vec.h"
#include "toys/vec_internal.h"

#include <string.h>

#define VEC_TEST_N 100

typedef struct {
    int id;
    char payload[128];
} VecBig;

#define VEC_ELEM VecBig
#define VEC_T WstVecBig
#define VEC_F(name) wst_vec_test_big_ ## name
#define VEC_REF
#define VEC_IMPL
#include "toys/vec_internal.h"
#undef VEC_IMPL

static void test_double_push_pop(void)
{
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_init(&s, 16) == WST_VEC_NO_ERR, "init ok");
    CHECK(s.length == 0 && s.cap >= 16, "init preallocates");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "empty verifies");

    for (int i = 0; i < VEC_TEST_N; i++)
        CHECK(wst_vec_double_push(&s, (double)i) == WST_VEC_NO_ERR, "push %d", i);
    CHECK(s.length == VEC_TEST_N, "len after pushes");
    CHECK(s.cap >= VEC_TEST_N, "cap grew");

    bool ok = true;
    for (int i = VEC_TEST_N - 1; i >= 0; i--) {
        double v = 0;
        if (wst_vec_double_pop(&s, &v) != WST_VEC_NO_ERR || !my_iszero(v - (double)i))
            ok = false;
    }
    CHECK(ok, "push pop order");
    CHECK(s.length == 0, "len after pops");

    CHECK(wst_vec_double_pop(&s, NULL) == WST_VEC_ERR_EMPTY, "pop empty errs");

    double v = 0;
    CHECK(wst_vec_double_push(&s, 7.0) == WST_VEC_NO_ERR, "push after empty");
    CHECK(wst_vec_double_pop(&s, NULL) == WST_VEC_NO_ERR, "pop with null out discards");
    CHECK(wst_vec_double_pop(&s, &v) == WST_VEC_ERR_EMPTY, "discarded value is gone");

    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok");
    CHECK(s.data == NULL && s.length == 0 && s.cap == 0, "free zeroes");
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "double free safe");
}

static void test_reserve(void)
{
    WstVecDouble s = { 0 };

    CHECK(wst_vec_double_reserve(&s, 64) == WST_VEC_NO_ERR, "reserve grows");
    CHECK(s.cap >= 64, "cap holds request");
    size_t cap = s.cap;

    CHECK(wst_vec_double_reserve(&s, 8) == WST_VEC_NO_ERR, "smaller reserve ok");
    CHECK(s.cap == cap, "smaller reserve is a no-op");

    CHECK(wst_vec_double_push(&s, 1.5) == WST_VEC_NO_ERR, "push after reserve");
    CHECK(my_iszero(s.data[0] - 1.5), "value kept");

    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok");
}

static void test_corrupt_tail(void)
{
#if VEC_USE_CANARY
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "intact verifies");

    unsigned char *tail = (unsigned char *)s.data + s.cap * sizeof(double);
    unsigned char saved = tail[0];
    tail[0] ^= 0xFF;

    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "damaged tail fails verify");
    CHECK(wst_vec_double_push(&s, 2.0) == WST_VEC_ERR_CORRUPT, "push refused when corrupt");
    CHECK(s.length == 1, "refused push keeps length");

    tail[0] = saved;
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "restored tail verifies");

    tail[0] ^= 0xFF;
    CHECK(wst_vec_double_free(&s) == WST_VEC_ERR_CORRUPT, "free refuses corruption");
    CHECK(s.data != NULL && s.length == 1, "refused free touches nothing");
    tail[0] = saved;
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok after restore");
#endif
}

static void test_corrupt_front(void)
{
#if VEC_USE_CANARY
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "intact verifies");

    unsigned char *front = (unsigned char *)s.data - VEC_CANARY_SIZE;
    unsigned char saved = front[0];
    front[0] ^= 0xFF;

    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "damaged front fails verify");
    CHECK(wst_vec_double_push(&s, 2.0) == WST_VEC_ERR_CORRUPT, "push refused when corrupt");
    CHECK(s.length == 1, "refused push keeps length");

    front[0] = saved;
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "restored front verifies");
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok after restore");
#endif
}

static void test_corrupt_length(void)
{
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");

    s.length = s.cap + 1;
    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "length over cap fails");

    double v = 0;
    CHECK(wst_vec_double_pop(&s, &v) == WST_VEC_ERR_CORRUPT, "pop refused when corrupt");

    s.length = 1;
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "fixed length verifies");
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok");
}

static void test_corrupt_hash(void)
{
#if VEC_USE_HASH
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_push(&s, 2.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_push(&s, 3.0) == WST_VEC_NO_ERR, "push");

    s.length = 1;
    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "in-bounds length fails");
    double v = 0;
    CHECK(wst_vec_double_pop(&s, &v) == WST_VEC_ERR_CORRUPT, "pop refused when corrupt");

    s.length = 3;
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "restored length verifies");

    s.hash ^= 0xFF;
    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "damaged hash fails");
    CHECK(wst_vec_double_free(&s) == WST_VEC_ERR_CORRUPT, "free refuses corruption");
    CHECK(s.data != NULL, "refused free touches nothing");
    s.hash ^= 0xFF;
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok after restore");
#endif
}

static void test_corrupt_content(void)
{
#if VEC_USE_CONTENT_HASH && VEC_USE_HASH
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_push(&s, 2.0) == WST_VEC_NO_ERR, "push");

    ((unsigned char *)s.data)[0] ^= 0xFF;

    wst_vec_double_dump(&s);
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "damaged content fails verify");
    double v = 0;
    CHECK(wst_vec_double_pop(&s, &v) == WST_VEC_ERR_CORRUPT, "pop refused when corrupt");

    ((unsigned char *)s.data)[0] ^= 0xFF;
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "restored content verifies");
    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok after restore");
#endif
}

static void test_init(void)
{
    WstVecDouble s = { 0 };
    CHECK(wst_vec_double_init(&s, 32) == WST_VEC_NO_ERR, "init reserves");
    CHECK(s.length == 0 && s.cap >= 32, "init leaves empty vector with cap");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "init verifies");

    CHECK(wst_vec_double_push(&s, 1.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_push(&s, 2.0) == WST_VEC_NO_ERR, "push");
    CHECK(wst_vec_double_init(&s, 64) == WST_VEC_NO_ERR, "reinit ok");
    CHECK(s.length == 0 && s.cap >= 64, "reinit drops data and grows");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_NO_ERR, "reinit verifies");

    double v = 0;
    CHECK(wst_vec_double_push(&s, 5.0) == WST_VEC_NO_ERR, "push after reinit");
    CHECK(wst_vec_double_pop(&s, &v) == WST_VEC_NO_ERR && my_iszero(v - 5.0), "old data gone");

    CHECK(wst_vec_double_push(&s, 6.0) == WST_VEC_NO_ERR, "push");
#if VEC_USE_CANARY
    unsigned char *tail = (unsigned char *)s.data + s.cap * sizeof(double);
    tail[0] ^= 0xFF;
    CHECK(wst_vec_double_init(&s, 16) == WST_VEC_ERR_CORRUPT, "init reports prior damage");
    CHECK(s.length == 1, "refused init touches nothing");
    CHECK(wst_vec_double_verify(&s) == WST_VEC_ERR_CORRUPT, "still corrupt");
    tail[0] ^= 0xFF;
    CHECK(wst_vec_double_init(&s, 16) == WST_VEC_NO_ERR, "init ok after restore");
    CHECK(s.length == 0 && s.cap >= 16, "init drops data and grows");
#endif

    CHECK(wst_vec_double_free(&s) == WST_VEC_NO_ERR, "free ok");
}

static void test_int(void)
{
    WstVecInt s = { 0 };
    CHECK(wst_vec_int_init(&s, 0) == WST_VEC_NO_ERR, "init ok");

    for (int i = 0; i < 20; i++)
        CHECK(wst_vec_int_push(&s, i * 3) == WST_VEC_NO_ERR, "push %d", i);

    bool ok = true;
    for (int i = 19; i >= 0; i--) {
        int v = 0;
        if (wst_vec_int_pop(&s, &v) != WST_VEC_NO_ERR || v != i * 3)
            ok = false;
    }
    CHECK(ok, "int push pop order");
    CHECK(wst_vec_int_free(&s) == WST_VEC_NO_ERR, "free ok");
}

static void test_big(void)
{
    WstVecBig s = { 0 };
    CHECK(wst_vec_test_big_init(&s, 4) == WST_VEC_NO_ERR, "big init");
    CHECK(wst_vec_test_big_verify(&s) == WST_VEC_NO_ERR, "big verifies");

    for (int i = 0; i < 10; i++) {
        VecBig v = { .id = i };
        memset(v.payload, i, sizeof(v.payload));
        CHECK(wst_vec_test_big_push(&s, &v) == WST_VEC_NO_ERR, "big push %d", i);
    }
    CHECK(s.length == 10, "big len");

    bool ok = true;
    for (int i = 9; i >= 0; i--) {
        VecBig v = { 0 };
        if (wst_vec_test_big_pop(&s, &v) != WST_VEC_NO_ERR || v.id != i)
            ok = false;
        for (size_t k = 0; k < sizeof(v.payload); k++)
            ok = ok && v.payload[k] == (char)i;
    }
    CHECK(ok, "big push pop order and payload intact");
    CHECK(wst_vec_test_big_free(&s) == WST_VEC_NO_ERR, "big free");
}

static void test_verify_graceful(void)
{
    CHECK(wst_vec_void_verify(NULL, sizeof(double)) == WST_VEC_ERR_NULL, "null vector");

    WstVecVoid empty = { 0 };
    CHECK(wst_vec_void_verify(&empty, 0) == WST_VEC_ERR_NULL, "zero element size");
    CHECK(wst_vec_void_verify(&empty, sizeof(double)) == WST_VEC_NO_ERR, "empty verifies");
    wst_vec_void_dump(NULL, sizeof(double), "WstVecVoid");
    wst_vec_void_dump(&empty, sizeof(double), "WstVecVoid");
}

static void test_err_str(void)
{
    WstVecErr codes[] = {
        WST_VEC_NO_ERR, WST_VEC_ERR_NOMEM, WST_VEC_ERR_EMPTY,
        WST_VEC_ERR_CORRUPT, WST_VEC_ERR_OVERFLOW, WST_VEC_ERR_NULL,
    };
    for (size_t i = 0; i < ARR_LEN(codes); i++)
        CHECK(wst_vec_err_str(codes[i]) != NULL, "err str %zu", i);
    CHECK(strcmp(wst_vec_err_str(WST_VEC_NO_ERR), "no error") == 0, "err str text");
}

static void test_zero_init(void)
{
    WstVecInt v = { 0 };
    CHECK(v.length == 0 && v.cap == 0, "zero init reads as empty");
    CHECK(wst_vec_int_verify(&v) == WST_VEC_NO_ERR, "zero init verifies");

    for (int i = 0; i < 100; i++)
        CHECK(wst_vec_int_push(&v, i * 2) == WST_VEC_NO_ERR, "push %d", i);
    CHECK(v.length == 100, "len after pushes");

    bool ok = true;
    for (int i = 0; i < 100; i++)
        ok = ok && wst_vec_int_get(&v, (size_t)i) == i * 2;
    CHECK(ok, "pushed values");

    CHECK(wst_vec_int_set(&v, 7, -1) == WST_VEC_NO_ERR, "write by index");
    CHECK(wst_vec_int_get(&v, 7) == -1, "read back");

    CHECK(wst_vec_int_free(&v) == WST_VEC_NO_ERR, "free ok");
    CHECK(v.data == NULL, "free nulls the buffer");
}

static void test_ensure(void)
{
    WstVecU8 v = { 0 };
    CHECK(wst_vec_u8_init(&v, 0) == WST_VEC_NO_ERR, "init ok");
    CHECK(wst_vec_u8_verify(&v) == WST_VEC_NO_ERR, "empty verifies");

    CHECK(wst_vec_u8_ensure(&v, 16) == WST_VEC_NO_ERR, "ensure grows");
    CHECK(v.length == 16, "length follows ensure");

    bool zeroed = true;
    for (size_t i = 0; i < v.length; i++)
        zeroed = zeroed && v.data[i] == 0;
    CHECK(zeroed, "new part is zero-filled");

    for (size_t i = 0; i < 8; i++)
        CHECK(wst_vec_u8_set(&v, i, (uint8_t)(i + 1)) == WST_VEC_NO_ERR, "set %zu", i);

    CHECK(wst_vec_u8_ensure(&v, 32) == WST_VEC_NO_ERR, "ensure grows again");
    bool kept = true;
    for (size_t i = 0; i < 8; i++)
        kept = kept && v.data[i] == (uint8_t)(i + 1);
    CHECK(kept, "old bytes kept");
    CHECK(v.data[31] == 0, "gap is zero-filled");

    CHECK(wst_vec_u8_ensure(&v, 4) == WST_VEC_NO_ERR, "smaller ensure ok");
    CHECK(v.length == 32, "smaller ensure is a no-op");

    CHECK(wst_vec_u8_free(&v) == WST_VEC_NO_ERR, "free ok");
    CHECK(v.data == NULL && v.length == 0 && v.cap == 0, "free zeroes");
}

static void test_indexed(void)
{
    WstVecU8 v = { 0 };
    CHECK(wst_vec_u8_init(&v, 4) == WST_VEC_NO_ERR, "init ok");

    for (uint32_t i = 0; i < 300; i++)
        CHECK(wst_vec_u8_push(&v, (uint8_t)i) == WST_VEC_NO_ERR, "push %u", i);
    CHECK(v.length == 300, "len after pushes");

    bool ok = true;
    for (uint32_t i = 0; i < 300; i++)
        ok = ok && v.data[i] == (uint8_t)i;
    CHECK(ok, "bytes round-trip");

    ok = true;
    for (uint32_t i = 0; i < 300; i++)
        ok = ok && wst_vec_u8_get(&v, i) == (uint8_t)i;
    CHECK(ok, "get reads back");

    uint8_t top = 0;
    CHECK(wst_vec_u8_pop(&v, &top) == WST_VEC_NO_ERR && top == (uint8_t)299, "pop");
    CHECK(wst_vec_u8_free(&v) == WST_VEC_NO_ERR, "free ok");
}

static void test_resize_clear(void)
{
    WstVecU8 v = { 0 };
    CHECK(wst_vec_u8_init(&v, 0) == WST_VEC_NO_ERR, "init ok");

    CHECK(wst_vec_u8_resize(&v, 10) == WST_VEC_NO_ERR, "resize grows");
    CHECK(v.length == 10, "length follows resize");
    CHECK(wst_vec_u8_get(&v, 9) == 0, "growth is zero-filled");

    CHECK(wst_vec_u8_set(&v, 0, 7) == WST_VEC_NO_ERR, "set");
    CHECK(wst_vec_u8_resize(&v, 4) == WST_VEC_NO_ERR, "resize shrinks");
    CHECK(v.length == 4, "length follows shrink");
    CHECK(wst_vec_u8_get(&v, 0) == 7, "kept bytes survive shrink");
    CHECK(wst_vec_u8_verify(&v) == WST_VEC_NO_ERR, "shrunk verifies");

    size_t cap = v.cap;
    CHECK(wst_vec_u8_clear(&v) == WST_VEC_NO_ERR, "clear ok");
    CHECK(v.length == 0 && v.cap == cap, "clear keeps capacity");
    CHECK(wst_vec_u8_verify(&v) == WST_VEC_NO_ERR, "cleared verifies");
    CHECK(wst_vec_u8_free(&v) == WST_VEC_NO_ERR, "free ok");
}

static void test_corrupt(void)
{
#if VEC_USE_CANARY
    WstVecU8 v = { 0 };
    CHECK(wst_vec_u8_ensure(&v, 8) == WST_VEC_NO_ERR, "ensure");

    unsigned char *tail = (unsigned char *)v.data + v.cap * sizeof(uint8_t);
    tail[0] ^= 0xFF;
    CHECK(wst_vec_u8_verify(&v) == WST_VEC_ERR_CORRUPT, "damaged tail fails");
    tail[0] ^= 0xFF;
    CHECK(wst_vec_u8_verify(&v) == WST_VEC_NO_ERR, "restored tail verifies");
    CHECK(wst_vec_u8_free(&v) == WST_VEC_NO_ERR, "free ok");
#endif
}

int main(void)
{
    test_double_push_pop();
    test_init();
    test_reserve();
    test_corrupt_tail();
    test_corrupt_front();
    test_corrupt_length();
    test_corrupt_hash();
    test_corrupt_content();
    test_int();
    test_big();
    test_verify_graceful();
    test_err_str();
    test_zero_init();
    test_ensure();
    test_indexed();
    test_resize_clear();
    test_corrupt();
    return tests_summary();
}
