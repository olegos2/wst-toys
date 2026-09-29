#include "tests_common.h"

#include "toys/stk.h"

#include <string.h>

#define STK_TEST_N 100

static void test_double_lifo(void)
{
    WstStkDouble s = { 0 };
    CHECK(wst_stk_double_init(&s) == WST_STK_NO_ERR, "init ok");
    CHECK(wst_stk_double_verify(&s) == WST_STK_NO_ERR, "empty verifies");

    for (int i = 0; i < STK_TEST_N; i++)
        CHECK(wst_stk_double_push(&s, (double)i) == WST_STK_NO_ERR, "push %d", i);
    CHECK(s.length == STK_TEST_N, "len after pushes");
    CHECK(s.cap >= STK_TEST_N, "cap grew");

    bool ok = true;
    for (int i = STK_TEST_N - 1; i >= 0; i--) {
        double v = 0;
        if (wst_stk_double_pop(&s, &v) != WST_STK_NO_ERR || !my_iszero(v - (double)i))
            ok = false;
    }
    CHECK(ok, "lifo order");
    CHECK(s.length == 0, "len after pops");

    CHECK(wst_stk_double_pop(&s, NULL) == WST_STK_ERR_EMPTY, "pop empty errs");

    double v = 0;
    CHECK(wst_stk_double_push(&s, 7.0) == WST_STK_NO_ERR, "push after empty");
    CHECK(wst_stk_double_pop(&s, NULL) == WST_STK_NO_ERR, "pop with null out discards");
    CHECK(wst_stk_double_pop(&s, &v) == WST_STK_ERR_EMPTY, "discarded value is gone");

    CHECK(wst_stk_double_free(&s) == WST_STK_NO_ERR, "free ok");
    CHECK(s.data == NULL && s.length == 0 && s.cap == 0, "free zeroes");
    CHECK(wst_stk_double_free(&s) == WST_STK_NO_ERR, "double free safe");
}

static void test_reserve(void)
{
    WstStkDouble s = { 0 };

    CHECK(wst_stk_double_reserve(&s, 64) == WST_STK_NO_ERR, "reserve grows");
    CHECK(s.cap >= 64, "cap holds request");
    size_t cap = s.cap;

    CHECK(wst_stk_double_reserve(&s, 8) == WST_STK_NO_ERR, "smaller reserve ok");
    CHECK(s.cap == cap, "smaller reserve is a no-op");

    CHECK(wst_stk_double_push(&s, 1.5) == WST_STK_NO_ERR, "push after reserve");
    CHECK(my_iszero(s.data[0] - 1.5), "value kept");

    CHECK(wst_stk_double_free(&s) == WST_STK_NO_ERR, "free ok");
}

static void test_corrupt_tail(void)
{
    WstStkDouble s = { 0 };
    CHECK(wst_stk_double_push(&s, 1.0) == WST_STK_NO_ERR, "push");
    CHECK(wst_stk_double_verify(&s) == WST_STK_NO_ERR, "intact verifies");

    unsigned char *tail = (unsigned char *)s.data + s.cap * sizeof(double);
    unsigned char saved = tail[0];
    tail[0] ^= 0xFF;

    CHECK(wst_stk_double_verify(&s) == WST_STK_ERR_CORRUPT, "damaged tail fails verify");
    CHECK(wst_stk_double_push(&s, 2.0) == WST_STK_ERR_CORRUPT, "push refused when corrupt");
    CHECK(s.length == 1, "refused push keeps length");

    tail[0] = saved;
    CHECK(wst_stk_double_verify(&s) == WST_STK_NO_ERR, "restored tail verifies");

    tail[0] ^= 0xFF;
    CHECK(wst_stk_double_free(&s) == WST_STK_ERR_CORRUPT, "free reports corruption");
    CHECK(s.data == NULL, "free still releases buffer");
}

static void test_corrupt_length(void)
{
    WstStkDouble s = { 0 };
    CHECK(wst_stk_double_push(&s, 1.0) == WST_STK_NO_ERR, "push");

    s.length = s.cap + 1;
    CHECK(wst_stk_double_verify(&s) == WST_STK_ERR_CORRUPT, "length over cap fails");

    double v = 0;
    CHECK(wst_stk_double_pop(&s, &v) == WST_STK_ERR_CORRUPT, "pop refused when corrupt");

    s.length = 1;
    CHECK(wst_stk_double_verify(&s) == WST_STK_NO_ERR, "fixed length verifies");
    CHECK(wst_stk_double_free(&s) == WST_STK_NO_ERR, "free ok");
}

static void test_int(void)
{
    WstStkInt s = { 0 };
    CHECK(wst_stk_int_init(&s) == WST_STK_NO_ERR, "init ok");

    for (int i = 0; i < 20; i++)
        CHECK(wst_stk_int_push(&s, i * 3) == WST_STK_NO_ERR, "push %d", i);

    bool ok = true;
    for (int i = 19; i >= 0; i--) {
        int v = 0;
        if (wst_stk_int_pop(&s, &v) != WST_STK_NO_ERR || v != i * 3)
            ok = false;
    }
    CHECK(ok, "int lifo order");
    CHECK(wst_stk_int_free(&s) == WST_STK_NO_ERR, "free ok");
}

static void test_err_str(void)
{
    WstStkErr codes[] = {
        WST_STK_NO_ERR, WST_STK_ERR_NOMEM, WST_STK_ERR_EMPTY,
        WST_STK_ERR_CORRUPT, WST_STK_ERR_OVERFLOW,
    };
    for (size_t i = 0; i < ARR_LEN(codes); i++)
        CHECK(wst_stk_err_str(codes[i]) != NULL, "err str %zu", i);
    CHECK(strcmp(wst_stk_err_str(WST_STK_NO_ERR), "no error") == 0, "err str text");
}

int main(void)
{
    test_double_lifo();
    test_reserve();
    test_corrupt_tail();
    test_corrupt_length();
    test_int();
    test_err_str();
    return tests_summary();
}
