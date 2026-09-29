
#define STK_IMPL
#include "toys/stk.h"

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
