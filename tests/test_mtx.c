#include "tests_common.h"

#include "toys/math.h"
#include "toys/mtx.h"

static bool cmp_mtx(const WstMtx *a, const double *expected)
{
    for (size_t i = 0; i < a->rows * a->cols; i++) {
        if (!my_iszero(a->arr[i] - expected[i]))
            return false;
    }
    return true;
}

static void print_mtx(const WstMtx *m, const char *name)
{
    fprintf(stderr, "%s (%zux%zu):", name, m->rows, m->cols);
    for (size_t i = 0; i < m->rows * m->cols; i++)
        fprintf(stderr, " %lg", m->arr[i]);
    fprintf(stderr, "\n");
}

static void print_expected(const double *data, size_t rows, size_t cols, const char *name)
{
    fprintf(stderr, "%s (%zux%zu):", name, rows, cols);
    for (size_t i = 0; i < rows * cols; i++)
        fprintf(stderr, " %lg", data[i]);
    fprintf(stderr, "\n");
}

static void test_add(void)
{
    static const struct {
        size_t a_rows, a_cols;
        double a[6];
        size_t b_rows, b_cols;
        double b[6];
        double exp[6];
        bool exp_ret;
    } cases[] = {
        {
            .a_rows = 2, .a_cols = 2,
            .b_rows = 2, .b_cols = 2,
            .a = { 1, 2, 3, 4 },
            .b = { 5, 6, 7, 8 },
            .exp = { 6, 8, 10, 12 },
            .exp_ret = true,
        },
        {
            .a_rows = 1, .a_cols = 3,
            .b_rows = 1, .b_cols = 3,
            .a = { 1, -2, 3 },
            .b = { -1, 2, -3 },
            .exp = { 0, 0, 0 },
            .exp_ret = true,
        },
        {
            /* dimension mismatch */
            .a_rows = 2, .a_cols = 2,
            .b_rows = 2, .b_cols = 3,
            .a = { 1, 2, 3, 4 },
            .b = { 1, 2, 3, 4, 5, 6 },
            .exp = { 0 },
            .exp_ret = false,
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        WstMtx a = { 0 }, b = { 0 };
        wst_mtx_create(cases[i].a, cases[i].a_rows, cases[i].a_cols, &a);
        wst_mtx_create(cases[i].b, cases[i].b_rows, cases[i].b_cols, &b);
        bool ret = wst_mtx_add(&a, &b);
        if (ret != cases[i].exp_ret) {
            FAIL("mtx add test #%zu: got ret %d, expected %d",
                 i + 1, ret, cases[i].exp_ret);
        } else if (ret && !cmp_mtx(&a, cases[i].exp)) {
            FAIL("mtx add test #%zu", i + 1);
            print_mtx(&a, "Got     ");
            print_expected(cases[i].exp, cases[i].a_rows, cases[i].a_cols, "Expected");
        }
        wst_mtx_clear(&a);
        wst_mtx_clear(&b);
    }
}

static void test_mul(void)
{
    static const struct {
        size_t a_rows, a_cols;
        double a[6];
        size_t b_rows, b_cols;
        double b[6];
        size_t exp_rows, exp_cols;
        double exp[4];
        bool exp_ret;
    } cases[] = {
        {
            /* 2x3 * 3x2 */
            .a_rows = 2, .a_cols = 3,
            .b_rows = 3, .b_cols = 2,
            .a = { 1, 2, 3, 4, 5, 6 },
            .b = { 7, 8, 9, 10, 11, 12 },
            .exp_rows = 2, .exp_cols = 2,
            .exp = { 58, 64, 139, 154 },
            .exp_ret = true,
        },
        {
            /* identity */
            .a_rows = 1, .a_cols = 1,
            .b_rows = 1, .b_cols = 1,
            .a = { 3 },
            .b = { 1 },
            .exp_rows = 1, .exp_cols = 1,
            .exp = { 3 },
            .exp_ret = true,
        },
        {
            /* dimension mismatch */
            .a_rows = 2, .a_cols = 3,
            .b_rows = 2, .b_cols = 2,
            .a = { 1, 2, 3, 4, 5, 6 },
            .b = { 1, 2, 3, 4 },
            .exp_rows = 0, .exp_cols = 0,
            .exp = { 0 },
            .exp_ret = false,
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        WstMtx a = { 0 }, b = { 0 }, res = { 0 };
        wst_mtx_create(cases[i].a, cases[i].a_rows, cases[i].a_cols, &a);
        wst_mtx_create(cases[i].b, cases[i].b_rows, cases[i].b_cols, &b);
        bool ret = wst_mtx_mul(&a, &b, &res);
        if (ret != cases[i].exp_ret) {
            FAIL("mtx mul test #%zu: got ret %d, expected %d",
                 i + 1, ret, cases[i].exp_ret);
        } else if (ret && (res.rows != cases[i].exp_rows || res.cols != cases[i].exp_cols ||
                           !cmp_mtx(&res, cases[i].exp))) {
            FAIL("mtx mul test #%zu", i + 1);
            print_mtx(&res, "Got     ");
            print_expected(cases[i].exp, cases[i].exp_rows, cases[i].exp_cols, "Expected");
        }
        wst_mtx_clear(&a);
        wst_mtx_clear(&b);
        wst_mtx_clear(&res);
    }
}

static void test_sym(void)
{
    static const struct {
        size_t size;
        double a[6];
        double b[6];
        double exp[6];
        bool exp_ret;
    } add_cases[] = {
        {
            .size = 2,
            .a = { 1, 2, 3 },
            .b = { 4, 5, 6 },
            .exp = { 5, 7, 9 },
            .exp_ret = true,
        },
        {
            /* dimension mismatch */
            .size = 2,
            .a = { 1, 2, 3 },
            .b = { 1, 2, 3 },
            .exp = { 0 },
            .exp_ret = true,
        },
    };

    for (size_t i = 0; i < ARR_LEN(add_cases); i++) {
        WstMtxSym a = { 0 }, b = { 0 };
        wst_mtxs_create(add_cases[i].a, add_cases[i].size, &a);
        if (i == 1) {
            /* mismatch: b has different size */
            wst_mtxs_create(add_cases[i].b, add_cases[i].size + 1, &b);
            bool ret = wst_mtxs_add(&a, &b);
            if (ret != false)
                FAIL("mtxs add test #%zu: got ret %d, expected 0", i + 1, ret);
        } else {
            wst_mtxs_create(add_cases[i].b, add_cases[i].size, &b);
            bool ret = wst_mtxs_add(&a, &b);
            bool ok = ret == add_cases[i].exp_ret;
            for (size_t j = 0; ok && j < add_cases[i].size * (add_cases[i].size + 1) / 2; j++)
                ok = my_iszero(a.arr[j] - add_cases[i].exp[j]);
            if (!ok) {
                FAIL("mtxs add test #%zu", i + 1);
                fprintf(stderr, "Got:     ");
                for (size_t j = 0; j < add_cases[i].size * (add_cases[i].size + 1) / 2; j++)
                    fprintf(stderr, " %lg", a.arr[j]);
                fprintf(stderr, "\nExpected:");
                for (size_t j = 0; j < add_cases[i].size * (add_cases[i].size + 1) / 2; j++)
                    fprintf(stderr, " %lg", add_cases[i].exp[j]);
                fprintf(stderr, "\n");
            }
        }
        wst_mtxs_clear(&a);
        wst_mtxs_clear(&b);
    }

    /* sym mul 2x2 identity-like check */
    {
        WstMtxSym a = { 0 }, b = { 0 };
        WstMtx res = { 0 };
        /* a = |1 2|, b = |0 1| => a*b = |2 3| */
        /*     |2 3|      |1 1|          |3 5| */
        double da[] = { 1, 2, 3 };
        double db[] = { 0, 1, 1 };
        double expected[] = { 2, 3, 3, 5 };
        wst_mtxs_create(da, 2, &a);
        wst_mtxs_create(db, 2, &b);
        bool ret = wst_mtxs_mul(&a, &b, &res);
        if (!ret || res.rows != 2 || res.cols != 2 || !cmp_mtx(&res, expected)) {
            FAIL("mtxs mul test #1");
            print_mtx(&res, "Got     ");
            print_expected(expected, 2, 2, "Expected");
        }
        wst_mtxs_clear(&a);
        wst_mtxs_clear(&b);
        wst_mtx_clear(&res);
    }
}

static void test_idx_edge(void)
{
    WstMtx m = { 0 };
    double data[] = { 1, 2, 3, 4 };
    wst_mtx_create(data, 2, 2, &m);
    CHECK(wst_mtx_idx(&m, 0, 0) != NULL, "idx in bounds");
    CHECK(wst_mtx_idx(&m, 2, 0) == NULL, "idx row out of bounds");
    CHECK(wst_mtx_idx(&m, 0, 2) == NULL, "idx col out of bounds");
    wst_mtx_clear(&m);
}

static void test_det(void)
{
    static const struct {
        size_t rows, cols;
        double data[9];
        double expected;
        bool exp_ret;
    } cases[] = {
        {
            /* 1x1 */
            .rows = 1, .cols = 1,
            .data = { 5 },
            .expected = 5, .exp_ret = true,
        },
        {
            /* 2x2 basic: 1*4 - 2*3 */
            .rows = 2, .cols = 2,
            .data = { 1, 2, 3, 4 },
            .expected = -2, .exp_ret = true,
        },
        {
            .rows = 2, .cols = 2,
            .data = { 0, 1, 1, 0 },
            .expected = -1, .exp_ret = true,
        },
        {
            /* singular */
            .rows = 2, .cols = 2,
            .data = { 1, 2, 2, 4 },
            .expected = 0, .exp_ret = true,
        },
        {
            /* 3x3: 1*(0-24) - 2*(0-20) + 3*(0-5) = 1 */
            .rows = 3, .cols = 3,
            .data = { 1, 2, 3, 0, 1, 4, 5, 6, 0 },
            .expected = 1, .exp_ret = true,
        },
        {
            /* 3x3 identity */
            .rows = 3, .cols = 3,
            .data = { 1, 0, 0, 0, 1, 0, 0, 0, 1 },
            .expected = 1, .exp_ret = true,
        },
        {
            /* not square */
            .rows = 2, .cols = 3,
            .data = { 1, 2, 3, 4, 5, 6 },
            .expected = 0, .exp_ret = false,
        },
    };

    for (size_t i = 0; i < ARR_LEN(cases); i++) {
        WstMtx m = { 0 };
        wst_mtx_create(cases[i].data, cases[i].rows, cases[i].cols, &m);
        double result = 0;
        bool ret = wst_mtx_det(&m, &result);
        if (ret != cases[i].exp_ret) {
            FAIL("mtx det test #%zu: got ret %d, expected %d",
                 i + 1, ret, cases[i].exp_ret);
        } else if (ret && !my_iszero(result - cases[i].expected)) {
            FAIL("mtx det test #%zu: got %lg, expected %lg",
                    i + 1, result, cases[i].expected);
            print_mtx(&m, "Matrix");
        }
        wst_mtx_clear(&m);
    }
}

int main(void)
{
    test_add();
    test_mul();
    test_sym();
    test_det();
    test_idx_edge();
    return tests_summary();
}
