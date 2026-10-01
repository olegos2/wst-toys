#ifndef TOYS_STK_H
#define TOYS_STK_H

#include <stddef.h>

/** Type to use for stack elements. */
#define STK_ELEM double

/** Name for stack struct. */
#define STK_T WstStkDouble

/** Func/var name generator in stack namespace. */
#define STK_F(name) wst_stk_double_ ## name

/** Stack element formatting. */
#define STK_FMT "%lg"

/** Generate stack for this element type. */
#include "stk_internal.h"


#define STK_ELEM int
#define STK_T WstStkInt
#define STK_F(name) wst_stk_int_ ## name
#define STK_FMT "%d"
#include "stk_internal.h"


#endif /* TOYS_STK_H */
