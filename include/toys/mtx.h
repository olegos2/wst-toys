#ifndef TOYS_MTX_H
#define TOYS_MTX_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Any dimensional matrix, `wst_mtx_*` functions should be used for ops.
 */
typedef struct {
    /** Dynamically allocated flat array of matrix data */
    double *arr;
    /** Matrix dimensions, should remain constant */
    size_t rows, cols;
} WstMtx;

/**
 * Matrix, but symmetrical (also square), saves storage by not storing duplicates.
 * Uses different layout and separate set of functions:
 * `wst_mtxs_*`.
 * Stores data as a pyramid/triangle of elements on and below main diagonal.
 */
typedef struct {
    /** Dynamically allocated flat array of matrix data */
    double *arr;
    /** Matrix dimensions, should remain constant */
    size_t size;
} WstMtxSym;

/**
 * Get matrix element pointer. Returns `NULL` on access out of bounds.
 */
double *wst_mtx_idx(const WstMtx *mtx, size_t row, size_t col);

/**
 * Get symmetrical matrix element pointer. Returns `NULL` on access out of bounds.
 */
double *wst_mtxs_idx(const WstMtxSym *mtx, size_t row, size_t col);

/**
 * Get matrix element with zero-indexed rows and columns.
 * Aborts on access out of bounds.
 */
double wst_mtx_get(const WstMtx *mtx, size_t row, size_t col);

/**
 * Get symmetrical matrix element with zero-indexed rows and columns.
 * Aborts on access out of bounds.
 */
double wst_mtxs_get(const WstMtxSym *mtx, size_t row, size_t col);

/**
 * Set matrix element, matrix must already be created.
 * Aborts on access out of bounds.
 */
void wst_mtx_set(const WstMtx *mtx, size_t row, size_t col, double val);

/**
 * Set symmetrical matrix element, matrix must already be created.
 * Aborts on access out of bounds.
 */
void wst_mtxs_set(const WstMtxSym *mtx, size_t row, size_t col, double val);

/**
 * `a += b` element-wise for matrices
 *
 * @return `false` when dimensions don't match, `true` otherwise.
 */
bool wst_mtx_add(WstMtx *a, const WstMtx *b);

/**
 * `a += b` element-wise for symmetrical matrices
 *
 * @return `false` when dimensions don't match, `true` otherwise.
 */
bool wst_mtxs_add(WstMtxSym *a, const WstMtxSym *b);

/**
 * `a -= b` element-wise for matrices
 *
 * @return `false` when dimensions don't match, `true` otherwise.
 */
bool wst_mtx_sub(WstMtx *a, const WstMtx *b);

/**
 * `a -= b` element-wise for symmetrical matrices
 *
 * @return `false` when dimensions don't match, `true` otherwise.
 */
bool wst_mtxs_sub(WstMtxSym *a, const WstMtxSym *b);

/**
 * `a *= k` element-wise scaling for matrix
 */
void wst_mtx_scale(WstMtx *a, double k);

/**
 * `a *= k` element-wise scaling for symmetrical matrix
 */
void wst_mtxs_scale(WstMtxSym *a, double k);

/**
 * `a = -a` element-wise for matrix
 */
void wst_mtx_neg(WstMtx *a);

/**
 * `a = -a` element-wise for symmetrical matrix
 */
void wst_mtxs_neg(WstMtxSym *a);

/**
 * `res = a * b` for matrices. `res` data will be reallocated and
 * result dimensions will be set.
 */
bool wst_mtx_mul(const WstMtx *a, const WstMtx *b, WstMtx *res);

/**
 * `res = a * b` for symmetrical matrices. `res` data will be reallocated and
 * result dimensions will be set.
 */
bool wst_mtxs_mul(const WstMtxSym *a, const WstMtxSym *b, WstMtx *res);

/**
 * Determinant of a square matrix, via Gaussian elimination
 * with partial pivoting.
 *
 * @param[in] mtx Square matrix.
 * @param[out] out Determinant value on success (0 for singular matrices).
 * @return `false` when the matrix is not square (or empty), `true` otherwise.
 */
bool wst_mtx_det(const WstMtx *mtx, double *out);

/**
 * Duplicate matrix.
 * Free what was in matrix `b`, duplicate `a` data into `b` and
 * copy dimensions of `a` to `b`.
 *
 * @param[in] a Matrix that needs to be duplicated.
 * @param[out] b Matrix that will be recreated to be a copy of `a`,
 *               struct must already be allocated.
 * @return `false` when allocation fails, `true` on success.
 */
bool wst_mtx_dup(const WstMtx *a, WstMtx *b);

/**
 * Duplicate symmetrical matrix.
 * Free what was in matrix `b`, duplicate `a` data into `b` and
 * copy dimensions of `a` to `b`.
 *
 * @param[in] a Matrix that needs to be duplicated.
 * @param[out] b Matrix that will be recreated to be a copy of `a`,
 *               struct must already be allocated.
 * @return `false` when allocation fails, `true` on success.
 */
bool wst_mtxs_dup(const WstMtxSym *a, WstMtxSym *b);

/**
 * Duplicate data from array representing matrix (stored column-first)
 * and its dimensions.
 *
 * @param[in] data Flattened column-first matrix data, or `NULL` for zero initialization.
 * @param[in] rows Number of rows in data.
 * @param[in] cols Number of columns in data.
 * @param[out] out Struct to store result in, must already be allocated.
 * @return `false` when allocation fails, `true` on success.
 */
bool wst_mtx_create(const double data[], size_t rows, size_t cols, WstMtx *out);

/**
 * Duplicate data from array representing symmetrical matrix (stored column-first)
 * and its dimensions.
 * `data` must 
 *
 * @param[in] data Flattened column-first matrix data, or `NULL` for zero initialization.
 * @param[in] size Dimensions of square matrix.
 * @param[out] out Struct to store result in, must already be allocated.
 * @return `false` when allocation fails, `true` on success.
 */
bool wst_mtxs_create(const double data[], size_t size, WstMtxSym *out);

/**
 * Frees array holding matrix data and sets dimensions to zeros.
 */
void wst_mtx_clear(WstMtx *a);

/**
 * Frees array holding symmetrical matrix data and sets size to zero.
 */
void wst_mtxs_clear(WstMtxSym *a);


#ifdef WST_MTX_SHORT_NAMES

#define mtx_idx wst_mtx_idx
#define mtxs_idx wst_mtxs_idx
#define mtx_get wst_mtx_get
#define mtxs_get wst_mtxs_get
#define mtx_set wst_mtx_set
#define mtxs_set wst_mtxs_set
#define mtx_add wst_mtx_add
#define mtxs_add wst_mtxs_add
#define mtx_sub wst_mtx_sub
#define mtxs_sub wst_mtxs_sub
#define mtx_scale wst_mtx_scale
#define mtxs_scale wst_mtxs_scale
#define mtx_neg wst_mtx_neg
#define mtxs_neg wst_mtxs_neg
#define mtx_mul wst_mtx_mul
#define mtxs_mul wst_mtxs_mul
#define mtx_det wst_mtx_det
#define mtx_dup wst_mtx_dup
#define mtxs_dup wst_mtxs_dup
#define mtx_create wst_mtx_create
#define mtxs_create wst_mtxs_create
#define mtx_clear wst_mtx_clear
#define mtxs_clear wst_mtxs_clear

#endif /* WST_MTX_SHORT_NAMES */

#endif /* TOYS_MTX_H */
