#include "toys/debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <string.h>

/** Currently saved file stream to use for printing logs to. */
static FILE *debug_file = NULL;

#ifdef WST_DEBUG
static ToysLogPrio debug_level = WST_LOG_DEBUG;
#else /* !WST_DEBUG */
static ToysLogPrio debug_level = WST_LOG_INFO;
#endif /* WST_DEBUG */

static void toys_log_close(void)
{
    if (debug_file != NULL) {
        fclose(debug_file);
        debug_file = NULL;
    }
}

bool toys_log_open(const char *filename)
{
    static bool close_registered = false;

    /* Ensure no file is left open/leaked. */
    toys_log_close();

    if (filename == NULL) {
        debug_file = NULL;
        return true;
    }

    debug_file = fopen(filename, "w");

    /* So flush happens on exit. */
    if (!close_registered) {
        atexit(toys_log_close);
        close_registered = true;
    }

    return debug_file != NULL;
}

int toys_log_print(ToysLogPrio prio, const char *fmt, ...)
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
        toys_log_close();
        va_start(args, fmt);
        ret = vfprintf(stderr, fmt, args);
        va_end(args);
    }

    if (s != stderr) fflush(s);
    return ret;
}

void toys_log_set_max_prio(ToysLogPrio prio)
{
    debug_level = prio;
}

static void print_stack_trace(void)
{
    static const int stack_len = 100;
    // TODO: use define
    void *buffer[stack_len];
    int cur_len = backtrace(buffer, stack_len);
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

void toys_assert(bool expr, const char *expr_src, const char *file,
                 int line, const char *func)
{
    if (expr) return;

    fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n",
            file, line, func, expr_src);
    print_stack_trace();
    abort();
}
