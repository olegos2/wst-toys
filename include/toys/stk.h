#ifndef TOYS_STK_H
#define TOYS_STK_H

#include <stddef.h>

/** Error that may occur while operating on a stack. */
typedef enum {
    WST_STK_NO_ERR = 0,
    /** Allocation failure, previous contents are kept. */
    WST_STK_ERR_NOMEM,
    /** Pop from an empty stack. */
    WST_STK_ERR_EMPTY,
    /** Tail canary or length/capacity invariant is damaged. */
    WST_STK_ERR_CORRUPT,
    /** Requested capacity does not fit into size_t. */
    WST_STK_ERR_OVERFLOW,
} WstStkErr;

/** Short human-readable description of a stack error. */
const char *wst_stk_err_str(WstStkErr err);

#define STK_ELEM double
#define STK_T WstStkDouble
#define STK_F(name) wst_stk_double_ ## name
#define STK_FMT "%lg"
#include "stk_internal.h"

#define STK_ELEM int
#define STK_T WstStkInt
#define STK_F(name) wst_stk_int_ ## name
#define STK_FMT "%d"
#include "stk_internal.h"

#endif /* TOYS_STK_H */
