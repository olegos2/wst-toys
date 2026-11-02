#include "debug_internal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ANSI colors. */
#define WST_COL_RESET     "\033[0m"
#define WST_COL_DIM       "\033[2m"
#define WST_COL_RED_B     "\033[1;31m"
#define WST_COL_YELLOW    "\033[33m"
#define WST_COL_YELLOW_B  "\033[1;33m"
#define WST_COL_GREEN_B   "\033[1;32m"
#define WST_COL_BLUE_B    "\033[1;34m"
#define WST_COL_MAGENTA   "\033[35m"
#define WST_COL_CYAN      "\033[36m"

const WstLogStyle wst_log_styles[WST_LOG_STYLE_COUNT] = {
    [WST_LOG_ERROR] = {
        .tag = "[E]",
        .html_class = "e",
        .html_body_class = "msg msg-error",
        .ansi = WST_COL_RED_B,
        .tint_body = true,
    },
    [WST_LOG_WARN] = {
        .tag = "[W]",
        .html_class = "w",
        .html_body_class = "msg msg-warn",
        .ansi = WST_COL_YELLOW_B,
        .tint_body = true,
    },
    [WST_LOG_INFO] = {
        .tag = "[I]",
        .html_class = "i",
        .html_body_class = "msg",
        .ansi = WST_COL_GREEN_B,
        .tint_body = false,
    },
    [WST_LOG_DEBUG] = {
        .tag = "[D]",
        .html_class = "d",
        .html_body_class = "msg",
        .ansi = WST_COL_BLUE_B,
        .tint_body = false,
    },
    [WST_LOG_VERBOSE] = {
        .tag = "[V]",
        .html_class = "v",
        .html_body_class = "msg",
        .ansi = WST_COL_DIM,
        .tint_body = false,
    },
};

static WstLogState log_state = {
    .stream = NULL,
    .format = WST_LOG_FORMAT_TEXT,
    .color_mode = WST_LOG_COLOR_AUTO,
#ifdef WST_DEBUG
    .max_prio = WST_LOG_DEBUG,
#else /* !WST_DEBUG */
    .max_prio = WST_LOG_INFO,
#endif /* WST_DEBUG */
    .dir = NULL,
    .path = NULL,
    .graph_path = NULL,
};

static bool atexit_registered = false;
static unsigned log_sequence;

WstLogState *wst_log_state(void)
{
    return &log_state;
}

FILE *wst_log_stream(void)
{
    return log_state.stream != NULL ? log_state.stream : stderr;
}

void wst_log_flush(void)
{
    if (log_state.stream != NULL)
        fflush(log_state.stream);
}

void wst_log_close(void)
{
    if (log_state.stream != NULL) {
        if (log_state.format == WST_LOG_FORMAT_HTML)
            fputs("</pre>\n</main>\n</body>\n</html>\n", log_state.stream);
        fflush(log_state.stream);
        fclose(log_state.stream);
    }

    free(log_state.dir);
    free(log_state.path);
    free(log_state.graph_path);
    log_state.dir = NULL;
    log_state.path = NULL;
    log_state.graph_path = NULL;
    log_state.stream = NULL;
    log_state.format = WST_LOG_FORMAT_TEXT;
}

static void log_atexit(void)
{
    wst_log_close();
}

bool wst_log_open(const char *log_dir, WstLogFormat format)
{
    wst_log_close();

    if (log_dir == NULL)
        return format != WST_LOG_FORMAT_HTML;

    if (mkdir(log_dir, 0777) != 0 && errno != EEXIST)
        return false;

    char *dir = strdup(log_dir);
    if (dir == NULL)
        return false;

    char stamp[WST_LOG_TIMESTAMP_CAP];
    wst_log_timestamp(stamp, sizeof(stamp), "%Y%m%d-%H%M%S");

    char display_stamp[WST_LOG_TIMESTAMP_CAP];
    wst_log_timestamp(display_stamp, sizeof(display_stamp),
                      "%Y-%m-%d %H:%M:%S");

    char name[96];
    snprintf(name, sizeof(name), "log-%s-%ld-%u", stamp, (long)getpid(),
             log_sequence++);

    const char *ext = format == WST_LOG_FORMAT_HTML ? ".html" : ".log";
    char *path = wst_log_path_join(dir, name, ext);
    if (path == NULL) {
        free(dir);
        return false;
    }

    FILE *stream = fopen(path, "wx");
    if (stream == NULL) {
        free(path);
        free(dir);
        return false;
    }

    log_state.dir = dir;
    log_state.path = path;
    log_state.stream = stream;
    log_state.format = format;

    if (format == WST_LOG_FORMAT_HTML)
        wst_log_html_header(stream, display_stamp);

    if (!atexit_registered) {
        atexit(log_atexit);
        atexit_registered = true;
    }
    return true;
}

const char *wst_log_path(void)
{
    return log_state.path;
}

WstLogFormat wst_log_format(void)
{
    return log_state.format;
}

bool wst_log_accepts_images(void)
{
    return log_state.format == WST_LOG_FORMAT_HTML && log_state.dir != NULL &&
           log_state.stream != NULL;
}

bool wst_log_use_color(FILE *stream)
{
    switch (log_state.color_mode) {
    case WST_LOG_COLOR_OFF:
        return false;
    case WST_LOG_COLOR_ON:
        return true;
    case WST_LOG_COLOR_AUTO:
    default:
        return stream == stderr && isatty(fileno(stderr));
    }
}

static int wst_log_text_line(WstLogPrio prio, const char *file,
                             const char *func, int line, const char *body)
{
    const WstLogStyle *style = &wst_log_styles[prio];
    FILE *stream = wst_log_stream();
    bool color = wst_log_use_color(stream);

    const char *file_fmt = color ? WST_COL_MAGENTA "%s" WST_COL_RESET : "%s";
    const char *func_fmt = color ? ":" WST_COL_CYAN "%s" WST_COL_RESET : ":%s";
    const char *line_fmt = color ? ":" WST_COL_DIM "%3d" WST_COL_RESET : ":%3d";
    const char *body_fmt = color && style->tint_body
                               ? WST_COL_YELLOW "%s" WST_COL_RESET
                               : "%s";

    int ret;
    if (color)
        ret = fprintf(stream, "%s%s%s ", style->ansi, style->tag,
                      WST_COL_RESET);
    else
        ret = fprintf(stream, "%s ", style->tag);

    if (file != NULL) {
        ret += fprintf(stream, file_fmt, file);
        if (func != NULL) {
            ret += fprintf(stream, func_fmt, func);
            if (line > 0)
                ret += fprintf(stream, line_fmt, line);
        }
        ret += fprintf(stream, ": ");
    }

    ret += fprintf(stream, body_fmt, body);
    return ret;
}

static int wst_log_vprint_at(WstLogPrio prio, const char *file,
                             const char *func, int line, const char *fmt,
                             va_list args)
{
    if (prio > log_state.max_prio)
        return 0;

    if (prio < 0 || prio > WST_LOG_VERBOSE)
        prio = WST_LOG_INFO;

    va_list measure;
    va_copy(measure, args);
    int body_len = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);
    if (body_len < 0)
        return -1;

    char *body = malloc((size_t)body_len + 1);
    if (body == NULL)
        return -1;
    vsnprintf(body, (size_t)body_len + 1, fmt, args);

    int ret = log_state.format == WST_LOG_FORMAT_HTML
                  ? wst_log_html_line(prio, file, func, line, body)
                  : wst_log_text_line(prio, file, func, line, body);

    free(body);
    wst_log_flush();
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
    LOG_V("new mode: %d", mode);
    log_state.color_mode = mode;
}

void wst_log_set_max_prio(WstLogPrio prio)
{
    log_state.max_prio = prio;
}
