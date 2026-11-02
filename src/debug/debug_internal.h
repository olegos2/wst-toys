#ifndef WST_DEBUG_INTERNAL_H
#define WST_DEBUG_INTERNAL_H

#include "toys/debug.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/** Logger state. A NULL stream means stderr. */
typedef struct {
    FILE *stream;
    WstLogFormat format;
    WstLogColorMode color_mode;
    WstLogPrio max_prio;
    char *dir;
    char *path;
    char *graph_path;
} WstLogState;

/** Presentation shared by text and HTML output. */
typedef struct {
    const char *tag;
    const char *html_class;
    const char *html_body_class;
    const char *ansi;
    bool tint_body;
} WstLogStyle;

#define WST_LOG_STYLE_COUNT (WST_LOG_VERBOSE + 1)

/* Fits the longest log timestamp format plus the terminator. */
#define WST_LOG_TIMESTAMP_CAP 32

extern const WstLogStyle wst_log_styles[WST_LOG_STYLE_COUNT];

WstLogState *wst_log_state(void);
FILE *wst_log_stream(void);
void wst_log_flush(void);
bool wst_log_use_color(FILE *stream);

void wst_log_html_escape(FILE *stream, const char *text);
void wst_log_html_header(FILE *stream, const char *stamp);
int wst_log_html_line(WstLogPrio prio, const char *file, const char *func,
                      int line, const char *body);

void wst_log_timestamp(char *out, size_t cap, const char *fmt);
char *wst_log_path_join(const char *dir, const char *name, const char *ext);
char *wst_log_sanitize_name(const char *name);

#endif /* WST_DEBUG_INTERNAL_H */
