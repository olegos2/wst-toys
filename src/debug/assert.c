#include "debug_internal.h"

#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_TRACE_DEPTH 100

static void print_stack_trace(void)
{
    void *buffer[STACK_TRACE_DEPTH];
    int cur_len = backtrace(buffer, STACK_TRACE_DEPTH);
    char **syms = backtrace_symbols(buffer, cur_len);
    if (syms == NULL) {
        LOG_E("backtrace_symbols: %s", strerror(errno));
        return;
    }
    fprintf(stderr, "Stack trace:\n");
    for (int i = 0; i < cur_len; i++)
        fprintf(stderr, "%s\n", syms[i]);
    free(syms);
}

void wst_assert(bool expr, const char *expr_src, const char *file,
                const char *func, int line)
{
    if (expr)
        return;

    fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n",
            file, line, func, expr_src);
    print_stack_trace();
    abort();
}
