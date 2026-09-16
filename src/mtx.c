#include "toys/mtx.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


double *wst_mtx_idx(const WstMtx *mtx, size_t row, size_t col)
{
    assert(mtx != NULL);

    if (row >= mtx->rows || col >= mtx->cols) {
        LOG_W("index (%zu, %zu) out of bounds for %zux%zu matrix",
              row, col, mtx->rows, mtx->cols);
        return NULL;
    }

    return &mtx->arr[row * mtx->cols + col];
}

double wst_mtx_get(const WstMtx *mtx, size_t row, size_t col)
{
    assert(row < mtx->rows);
    assert(col < mtx->cols);
    return *wst_mtx_idx(mtx, row, col);
}

void wst_mtx_set(const WstMtx *mtx, size_t row, size_t col, double val)
{
    assert(row < mtx->rows);
    assert(col < mtx->cols);
    *wst_mtx_idx(mtx, row, col) = val;
}

bool wst_mtx_add(WstMtx *a, const WstMtx *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->rows != b->rows || a->cols != b->cols) {
        LOG_D("cannot add %zux%zu + %zux%zu: dimension mismatch",
              a->rows, a->cols, b->rows, b->cols);
        return false;
    }

    for (size_t i = 0; i < a->rows * a->cols; i++)
        a->arr[i] += b->arr[i];

    return true;
}

bool wst_mtx_sub(WstMtx *a, const WstMtx *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->rows != b->rows || a->cols != b->cols) {
        LOG_D("cannot sub %zux%zu - %zux%zu: dimension mismatch",
              a->rows, a->cols, b->rows, b->cols);
        return false;
    }

    for (size_t i = 0; i < a->rows * a->cols; i++)
        a->arr[i] -= b->arr[i];

    return true;
}

void wst_mtx_scale(WstMtx *a, double k)
{
    assert(a != NULL);

    for (size_t i = 0; i < a->rows * a->cols; i++)
        a->arr[i] *= k;
}

void wst_mtx_neg(WstMtx *a)
{
    assert(a != NULL);

    for (size_t i = 0; i < a->rows * a->cols; i++)
        a->arr[i] = -a->arr[i];
}

static bool wst_mtx_resize(WstMtx *a, size_t rows, size_t cols)
{
    assert(a != NULL);

    if (a->rows == rows && a->cols == cols)
        return true;

    size_t res_len = rows * cols;

    if (res_len > 0) {
        double *new_arr = realloc(a->arr, res_len * sizeof(double));
        if (new_arr == NULL) {
            LOG_W("realloc: %s", strerror(errno));
            return false;
        }

        a->arr = new_arr;
        a->cols = cols;
        a->rows = rows;
    } else {
        wst_mtx_clear(a);
    }

    return true;
}

bool wst_mtx_mul(const WstMtx *a, const WstMtx *b, WstMtx *res)
{
    assert(a != NULL);
    assert(b != NULL);
    assert(res != NULL);

    if (a->cols != b->rows) {
        LOG_D("cannot mul %zux%zu * %zux%zu: inner dimensions mismatch",
              a->rows, a->cols, b->rows, b->cols);
        return false;
    }

    if (!wst_mtx_resize(res, a->rows, b->cols))
        return false;

    for (size_t row = 0; row < res->rows; row++) {
        for (size_t col = 0; col < res->cols; col++) {
            double val = 0;
            for (size_t pair = 0; pair < a->cols; pair++) {
                val += wst_mtx_get(a, row, pair) * wst_mtx_get(b, pair, col);
            }
            wst_mtx_set(res, row, col, val);
        }
    }

    return true;
}

bool wst_mtx_det(const WstMtx *mtx, double *out)
{
    assert(mtx != NULL);
    assert(out != NULL);

    if (mtx->rows != mtx->cols) {
        LOG_D("cannot det %zux%zu: matrix is not square",
              mtx->rows, mtx->cols);
        return false;
    }

    size_t n = mtx->rows;
    if (n == 0) {
        LOG_D("cannot det 0x0: matrix is empty");
        return false;
    }

    if (n == 1) {
        *out = mtx->arr[0];
        return true;
    }

    /* TODO */

    return false;
}

bool wst_mtx_dup(const WstMtx *a, WstMtx *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (!wst_mtx_resize(b, a->rows, a->cols))
        return false;

    for (size_t i = 0; i < a->rows * a->cols; i++)
        b->arr[i] = a->arr[i];

    return true;
}

bool wst_mtx_create(const double data[], size_t rows, size_t cols, WstMtx *out)
{
    assert(out != NULL);

    if (!wst_mtx_resize(out, rows, cols))
        return false;

    if (data == NULL)
        for (size_t i = 0; i < rows * cols; i++)
            out->arr[i] = 0.0;
    else
        for (size_t i = 0; i < rows * cols; i++)
            out->arr[i] = data[i];

    return true;
}

void wst_mtx_clear(WstMtx *a)
{
    assert(a != NULL);

    free(a->arr);
    a->arr = NULL;
    a->cols = 0;
    a->rows = 0;
}

/* Symmetrical/triangle matrices. */

#define MTXS_LEN(a) (a->size * (a->size + 1) / 2)

double *wst_mtxs_idx(const WstMtxSym *mtx, size_t row, size_t col)
{
    assert(mtx != NULL);

    if (row >= mtx->size || col >= mtx->size)
        return NULL;

    /* Swap col <-> row (upper half is not stored) */
    if (col > row)
        return &mtx->arr[(col + 1) * col / 2 + row];

    return &mtx->arr[(row + 1) * row / 2 + col];
}

double wst_mtxs_get(const WstMtxSym *mtx, size_t row, size_t col)
{
    assert(mtx != NULL);
    assert(row < mtx->size);
    assert(col < mtx->size);

    return *wst_mtxs_idx(mtx, row, col);
}

void wst_mtxs_set(const WstMtxSym *mtx, size_t row, size_t col, double val)
{
    assert(mtx != NULL);
    assert(row < mtx->size);
    assert(col < mtx->size);

    *wst_mtxs_idx(mtx, row, col) = val;
}

bool wst_mtxs_add(WstMtxSym *a, const WstMtxSym *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->size != b->size) {
        LOG_D("cannot add sym(%zu) + sym(%zu): size mismatch",
              a->size, b->size);
        return false;
    }

    for (size_t i = 0; i < MTXS_LEN(a); i++)
        a->arr[i] += b->arr[i];

    return true;
}

bool wst_mtxs_sub(WstMtxSym *a, const WstMtxSym *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->size != b->size) {
        LOG_D("cannot sub sym(%zu) - sym(%zu): size mismatch",
              a->size, b->size);
        return false;
    }

    for (size_t i = 0; i < MTXS_LEN(a); i++)
        a->arr[i] -= b->arr[i];

    return true;
}

void wst_mtxs_scale(WstMtxSym *a, double k)
{
    assert(a != NULL);

    for (size_t i = 0; i < MTXS_LEN(a); i++)
        a->arr[i] *= k;
}

void wst_mtxs_neg(WstMtxSym *a)
{
    assert(a != NULL);

    for (size_t i = 0; i < MTXS_LEN(a); i++)
        a->arr[i] = -a->arr[i];
}

static bool wst_mtxs_resize(WstMtxSym *a, size_t size)
{
    assert(a != NULL);

    if (a->size == size)
        return true;

    size_t res_len = size * (size + 1) / 2;

    if (res_len > 0) {
        double *new_arr = realloc(a->arr, res_len * sizeof(double));
        if (new_arr == NULL) {
            LOG_W("realloc: %s", strerror(errno));
            return false;
        }

        a->arr = new_arr;
        a->size = size;
    } else {
        wst_mtxs_clear(a);
    }

    return true;
}

bool wst_mtxs_mul(const WstMtxSym *a, const WstMtxSym *b, WstMtx *res)
{
    assert(a != NULL);
    assert(b != NULL);
    assert(res != NULL);

    if (a->size != b->size) {
        LOG_D("cannot mul sym(%zu) * sym(%zu): size mismatch", a->size, b->size);
        return false;
    }

    /* Result is not symmetrical:
     * | a b c |   | g h i |   | a*g + b*h + c*i   a*h + b*j + c*k   a*i + b*k + c*l |
     * | b d e | * | h j k | = | b*g + d*h + e*i   b*h + d*j + e*k   b*i + d*k + e*l |
     * | c e f |   | i k l |   | c*g + e*h + f*i   c*h + e*j + f*k   c*i + e*k + f*l |
     */

    if (!wst_mtx_resize(res, a->size, b->size))
        return false;

    for (size_t row = 0; row < res->rows; row++) {
        for (size_t col = 0; col < res->cols; col++) {
            double val = 0;
            for (size_t pair = 0; pair < a->size; pair++) {
                val += wst_mtxs_get(a, row, pair) * wst_mtxs_get(b, pair, col);
            }
            wst_mtx_set(res, row, col, val);
        }
    }

    return true;
}

bool wst_mtxs_dup(const WstMtxSym *a, WstMtxSym *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (!wst_mtxs_resize(b, a->size))
        return false;

    for (size_t i = 0; i < MTXS_LEN(a); i++) {
        b->arr[i] = a->arr[i];
    }

    return true;
}

bool wst_mtxs_create(const double data[], size_t size, WstMtxSym *out)
{
    assert(out != NULL);

    if (!wst_mtxs_resize(out, size))
        return false;

    if (data == NULL) {
        for (size_t i = 0; i < MTXS_LEN(out); i++)
            out->arr[i] = 0.0;
    } else {
        for (size_t i = 0; i < MTXS_LEN(out); i++)
            out->arr[i] = data[i];
    }

    return true;
}

void wst_mtxs_clear(WstMtxSym *a)
{
    assert(a != NULL);

    free(a->arr);
    a->arr = NULL;
    a->size = 0;
}
