#include "toys/debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <string.h>

#define STACK_TRACE_DEPTH 100

/** Currently saved file stream to use for printing logs to. */
static FILE *debug_file = NULL;

#ifdef WST_DEBUG
static WstLogPrio debug_level = WST_LOG_DEBUG;
#else /* !WST_DEBUG */
static WstLogPrio debug_level = WST_LOG_INFO;
#endif /* WST_DEBUG */

static void wst_log_close(void)
{
    if (debug_file != NULL) {
        fclose(debug_file);
        debug_file = NULL;
    }
}

bool wst_log_open(const char *filename)
{
    static bool close_registered = false;

    /* Ensure no file is left open/leaked. */
    wst_log_close();

    if (filename == NULL) {
        debug_file = NULL;
        return true;
    }

    debug_file = fopen(filename, "w");

    /* So flush happens on exit. */
    if (!close_registered) {
        atexit(wst_log_close);
        close_registered = true;
    }

    return debug_file != NULL;
}

int wst_log_print(WstLogPrio prio, const char *fmt, ...)
{
    if (prio > debug_level)
        return 0;
    va_list args = { 0 };
    va_start(args, fmt);
    FILE *s = debug_file ?: stderr;
    int ret = vfprintf(s, fmt, args);
    va_end(args);

    if (ret < 0) {
        fprintf(stderr, "Failed to print log to stream (fd %d), "
                "switching to stderr\n", fileno(s));
        wst_log_close();
        va_start(args, fmt);
        ret = vfprintf(stderr, fmt, args);
        va_end(args);
    }

    if (s != stderr) fflush(s);
    return ret;
}

void wst_log_set_max_prio(WstLogPrio prio)
{
    debug_level = prio;
}

static void print_stack_trace(void)
{
    void *buffer[STACK_TRACE_DEPTH];
    int cur_len = backtrace(buffer, STACK_TRACE_DEPTH);
    char **syms = backtrace_symbols(buffer, cur_len);
    if (syms == NULL) {
        LOG_E("backtrace_symbols: %s", strerror(errno));
        // perror("backtrace_symbols");
        return;
    }
    fprintf(stderr, "Stack trace:\n");
    for (int i = 0; i < cur_len; i++) {
        fprintf(stderr, "%s\n", syms[i]);
    }
    free(syms);
}

void wst_assert(bool expr, const char *expr_src, const char *file,
                 int line, const char *func)
{
    if (expr) return;

    fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n",
            file, line, func, expr_src);
    print_stack_trace();
    abort();
}
