#ifndef WST_DEBUG_H
#define WST_DEBUG_H

#include <stdbool.h>
#include <stddef.h>

#define __WST_LOG(prio, fmt, ...) \
    wst_log_print_at(WST_LOG_ ## prio, NULL, NULL, 0, fmt "\n", ## __VA_ARGS__)
    // wst_log_print(WST_LOG_ ## prio, fmt "\n", ## __VA_ARGS__)
    // wst_log_print(WST_LOG_ ## prio, "%s " fmt "\n", __wst_log_prio_fmt[WST_LOG_ ## prio], ## __VA_ARGS__)
    // fprintf(stderr, "%s: " fmt "\n", #prio, ## __VA_ARGS__)

#define __WST_LOG_FILE(prio, fmt, ...) \
    wst_log_print_at(WST_LOG_ ## prio, __FILE_NAME__, NULL, 0, fmt "\n", ## __VA_ARGS__)

#define __WST_LOG_FUNC(prio, fmt, ...) \
    wst_log_print_at(WST_LOG_ ## prio, __FILE_NAME__, __func__, 0, fmt "\n", ## __VA_ARGS__)

#define __WST_LOG_LINE(prio, fmt, ...) \
    wst_log_print_at(WST_LOG_ ## prio, __FILE_NAME__, __func__, __LINE__, fmt "\n", ## __VA_ARGS__)


#ifdef WST_DEBUG

/** Formatted verbose msg with file name, function, line num */
#define LOG_V(...) \
    __WST_LOG_LINE(VERBOSE, __VA_ARGS__)

/** Formatted debug msg with file name, function, line num */
#define LOG_D(...) \
    __WST_LOG_LINE(DEBUG, __VA_ARGS__)

#else /* !WST_DEBUG */

#define LOG_V(...) 0
#define LOG_D(...) 0

#endif /* WST_DEBUG */

/** Formatted warning with file name, function name */
#define LOG_W(...) \
    __WST_LOG_FUNC(WARN, __VA_ARGS__)

/** Formatted error with file name, function name */
#define LOG_E(...) \
    __WST_LOG_FUNC(ERROR, __VA_ARGS__)

/** Formatted info with file name. */
#define LOG_I(...) \
    __WST_LOG_FILE(INFO, __VA_ARGS__)

/** Log channels in ascending 'verbosity' */
typedef enum {
    WST_LOG_ERROR = 0,
    WST_LOG_WARN,
    WST_LOG_INFO,
    WST_LOG_DEBUG,
    WST_LOG_VERBOSE,
} WstLogPrio;

/** Color mode for log output */
typedef enum {
    /** Never use colors */
    WST_LOG_COLOR_OFF = 0,
    /** Always use colors */
    WST_LOG_COLOR_ON,
    /** Use colors only when logging to a terminal */
    WST_LOG_COLOR_AUTO,
} WstLogColorMode;


/**
 * Open a file and redirect all following logs to it.
 * When `filename` is NULL, prints to stderr.
 * By default logs are printed to stderr.
 */
bool wst_log_open(const char *filename);

/**
 * Print a log line to current stream. Filters by priority.
 *
 * @return Number of bytes written to stream.
 */
int wst_log_print(WstLogPrio prio, const char *fmt, ...);

/**
 * Print a log line with explicit source location.
 * File/func/line are rendered (and colored) by the logger itself.
 *
 * @param[in] file File name or NULL to omit.
 * @param[in] func Function name or NULL to omit.
 * @param[in] line Line number, ignored unless file and func are set.
 * @return Number of bytes written to stream.
 */
int wst_log_print_at(WstLogPrio prio, const char *file, const char *func,
                     int line, const char *fmt, ...);

/**
 * Set ANSI color output for logs. Default is auto.
 */
void wst_log_enable_color(WstLogColorMode mode);

/**
 * Sets max priority messages of which will be printed.
 */
void wst_log_set_max_prio(WstLogPrio prio);

/**
 * Custom assert impl that may be more verbose.
 * Aborts program when expression is false.
 *
 * @param[in] expr Expression evaluation result.
 * @param[in] expr_src Expression itself as a string.
 * @param[in] file File from which expression comes from.
 * @param[in] func Function in which this expression is located.
 * @param[in] line Line at which this expression is located.
 */
void wst_assert(bool expr, const char *expr_src, const char *file, const char *func, int line);

#ifndef NDEBUG
#  define my_assert(expr) wst_assert(expr, #expr, __FILE__, __func__, __LINE__)
#endif

#endif /* WST_DEBUG_H */
