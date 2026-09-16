#include "toys/debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <string.h>
#include <unistd.h>

#define STACK_TRACE_DEPTH 100

/* ANSI colors */
#define WST_COL_RESET     "\033[0m"
#define WST_COL_DIM       "\033[2m"
#define WST_COL_RED_B     "\033[1;31m"
#define WST_COL_YELLOW    "\033[33m"
#define WST_COL_YELLOW_B  "\033[1;33m"
#define WST_COL_GREEN     "\033[32m"
#define WST_COL_GREEN_B   "\033[1;32m"
#define WST_COL_BLUE      "\033[34m"
#define WST_COL_BLUE_B    "\033[1;34m"
#define WST_COL_MAGENTA   "\033[35m"
#define WST_COL_MAGENTA_B "\033[1;35m"
#define WST_COL_CYAN      "\033[36m"
#define WST_COL_CYAN_B    "\033[1;36m"

/** Currently saved file stream to use for printing logs to. */
static FILE *debug_file = NULL;

static WstLogColorMode color_mode = WST_LOG_COLOR_AUTO;

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
    /* Ensure no file is left open/leaked. */
    wst_log_close();

    if (filename == NULL)
        return true;

    debug_file = fopen(filename, "w");

    return debug_file != NULL;
}

static bool wst_log_use_color(FILE *stream)
{
    switch (color_mode) {
    case WST_LOG_COLOR_OFF:
        return false;
    case WST_LOG_COLOR_ON:
        return true;
    case WST_LOG_COLOR_AUTO:
    default:
        return stream == stderr && isatty(fileno(stderr));
    }
}

static int wst_log_vprint_at(WstLogPrio prio, const char *file, const char *func,
                             int line, const char *fmt, va_list args)
{
    static const char *prio_str[] = {
        [WST_LOG_ERROR] = "[E]",
        [WST_LOG_WARN] = "[W]",
        [WST_LOG_INFO] = "[I]",
        [WST_LOG_DEBUG] = "[D]",
        [WST_LOG_VERBOSE] = "[V]",
    };
    static const char *prio_color[] = {
        [WST_LOG_ERROR] = WST_COL_RED_B,
        [WST_LOG_WARN] = WST_COL_YELLOW_B,
        [WST_LOG_INFO] = WST_COL_GREEN_B,
        [WST_LOG_DEBUG] = WST_COL_BLUE_B,
        [WST_LOG_VERBOSE] = WST_COL_DIM,
    };

    if (prio > debug_level)
        return 0;

    FILE *stream = debug_file ?: stderr;
    bool color = wst_log_use_color(stream);
    int ret = 0;

    if (prio < 0 || prio > WST_LOG_VERBOSE)
        prio = WST_LOG_INFO;

    if (color)
        ret += fprintf(stream, "%s%s%s ", prio_color[prio], prio_str[prio], WST_COL_RESET);
    else
        ret += fprintf(stream, "%s ", prio_str[prio]);

    if (file != NULL) {
        if (color)
            ret += fprintf(stream, WST_COL_MAGENTA "%s" WST_COL_RESET, file);
        else
            ret += fprintf(stream, "%s", file);
        if (func != NULL) {
            if (color)
                ret += fprintf(stream, ":" WST_COL_CYAN "%s" WST_COL_RESET, func);
            else
                ret += fprintf(stream, ":%s", func);
            if (line > 0) {
                if (color)
                    ret += fprintf(stream, ":" WST_COL_DIM "%3d" WST_COL_RESET, line);
                else
                    ret += fprintf(stream, ":%3d", line);
            }
        }
        ret += fprintf(stream, ": ");
    }

    bool tint_body = color && (prio == WST_LOG_WARN || prio == WST_LOG_ERROR);
    if (tint_body)
        ret += fprintf(stream, "%s", WST_COL_YELLOW);

    ret += vfprintf(stream, fmt, args);

    if (tint_body)
        ret += fprintf(stream, "%s", WST_COL_RESET);

    if (stream != stderr)
        fflush(stream);
    return ret;
}

int wst_log_print_at(WstLogPrio prio, const char *file, const char *func,
                     int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = wst_log_vprint_at(prio, file, func, line, fmt, args);
    va_end(args);
    return ret;
}

int wst_log_print(WstLogPrio prio, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = wst_log_vprint_at(prio, NULL, NULL, 0, fmt, args);
    va_end(args);
    return ret;
}

void wst_log_enable_color(WstLogColorMode mode)
{
    color_mode = mode;
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
                const char *func, int line)
{
    if (expr) return;

    fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n",
            file, line, func, expr_src);
    print_stack_trace();
    abort();
}
