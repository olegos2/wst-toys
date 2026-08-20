#ifndef TOYS_DEBUG_H
#define TOYS_DEBUG_H

#include <stdio.h>

#define __TOYS_LOG(prio, fmt, ...) \
    fprintf(stderr, "%s: " fmt "\n", #prio, ## __VA_ARGS__)

#define __TOYS_LOG_FILE(prio, fmt, ...) \
    __TOYS_LOG(prio, "%s: " fmt, __FILE_NAME__, ## __VA_ARGS__)

#define __TOYS_LOG_FUNC(prio, fmt, ...) \
    __TOYS_LOG(prio, "%s:%s: " fmt, __FILE_NAME__, __func__, ## __VA_ARGS__)

#define __TOYS_LOG_LINE(prio, fmt, ...) \
    __TOYS_LOG(prio, "%s:%s:%d: " fmt, __FILE_NAME__, __func__, __LINE__, ## __VA_ARGS__)


#ifdef TOYS_DEBUG

/** Formatted verbose msg with file name, function, line num */
#define LOG_V(...) \
    __TOYS_LOG_LINE(VERBOSE, __VA_ARGS__)

/** Formatted debug msg with file name, function, line num */
#define LOG_D(...) \
    __TOYS_LOG_LINE(DEBUG, __VA_ARGS__)

#else /* !TOYS_DEBUG */

#define LOG_V(...) 0
#define LOG_D(...) 0

#endif /* TOYS_DEBUG */

/** Formatted warning with file name, function name */
#define LOG_W(...) \
    __TOYS_LOG_FUNC(WARN, __VA_ARGS__)

/** Formatted error with file name, function name */
#define LOG_E(...) \
    __TOYS_LOG_FUNC(ERROR, __VA_ARGS__)

/** Formatted info with file name. */
#define LOG_I(...) \
    __TOYS_LOG_FILE(INFO, __VA_ARGS__)

#endif /* TOYS_DEBUG_H */
