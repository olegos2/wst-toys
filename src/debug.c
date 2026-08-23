#include "toys/debug.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/** Currently saved file stream to use for printing logs to. */
static FILE *debug_file = NULL;
static ToysLogPrio debug_level = WST_LOG_INFO;

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
