#include "debug_internal.h"

#include <stdio.h>
#include <string.h>

static void html_escape_n(FILE *stream, const char *text, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        switch ((unsigned char)text[i]) {
        case '&':
            fputs("&amp;", stream);
            break;
        case '<':
            fputs("&lt;", stream);
            break;
        case '>':
            fputs("&gt;", stream);
            break;
        case '"':
            fputs("&quot;", stream);
            break;
        case '\'':
            fputs("&#39;", stream);
            break;
        default:
            fputc((unsigned char)text[i], stream);
            break;
        }
    }
}

void wst_log_html_escape(FILE *stream, const char *text)
{
    if (text != NULL)
        html_escape_n(stream, text, strlen(text));
}

void wst_log_html_header(FILE *stream, const char *stamp)
{
    fputs("<!doctype html>\n"
          "<html lang=\"en\">\n"
          "<head>\n"
          "<meta charset=\"utf-8\">\n"
          "<meta name=\"viewport\" "
          "content=\"width=device-width, initial-scale=1\">\n"
          "<meta name=\"theme-color\" content=\"#141218\">\n"
          "<title>wst-toys log ",
          stream);
    wst_log_html_escape(stream, stamp);
    fputs("</title>\n"
          "<style>\n"
          ":root {\n"
          "  color-scheme: dark;\n"
          "  --surface: #1c1b1f;\n"
          "  --surface-dim: #141218;\n"
          "  --surface-low: #1d1b20;\n"
          "  --surface-container: #211f26;\n"
          "  --surface-highest: #36343b;\n"
          "  --on-surface: #e6e1e5;\n"
          "  --on-surface-variant: #cac4d0;\n"
          "  --outline: #938f99;\n"
          "  --outline-variant: #49454f;\n"
          "  --primary: #d0bcff;\n"
          "  --primary-container: #4f378b;\n"
          "  --on-primary-container: #eaddff;\n"
          "  --secondary: #ccc2dc;\n"
          "  --secondary-container: #4a4458;\n"
          "  --on-secondary-container: #e8def8;\n"
          "  --tertiary: #efb8c8;\n"
          "  --tertiary-container: #633b48;\n"
          "  --on-tertiary-container: #ffd8e4;\n"
          "  --error: #f2b8b5;\n"
          "  --error-container: #8c1d18;\n"
          "  --on-error-container: #f9dedc;\n"
          "  --shape-sm: 8px;\n"
          "  --shape-md: 12px;\n"
          "  --sans: Roboto, \"Noto Sans\", system-ui, -apple-system, "
          "sans-serif;\n"
          "  --mono: \"Roboto Mono\", \"JetBrains Mono\", "
          "\"SFMono-Regular\", Consolas, monospace;\n"
          "}\n"
          "* { box-sizing: border-box; }\n"
          "html {\n"
          "  min-height: 100%;\n"
          "  background: var(--surface-dim);\n"
          "  scrollbar-color: var(--outline-variant) var(--surface-dim);\n"
          "  scrollbar-width: thin;\n"
          "}\n"
          "body {\n"
          "  min-height: 100vh;\n"
          "  margin: 0;\n"
          "  color: var(--on-surface);\n"
          "  font-family: var(--sans);\n"
          "  background: var(--surface-dim);\n"
          "}\n"
          "::selection {\n"
          "  background: var(--primary-container);\n"
          "  color: var(--on-primary-container);\n"
          "}\n"
          ".shell {\n"
          "  width: min(1240px, calc(100% - 32px));\n"
          "  margin: 16px auto 24px;\n"
          "  overflow: hidden;\n"
          "  background: var(--surface-low);\n"
          "  border: 1px solid var(--outline-variant);\n"
          "  border-radius: var(--shape-md);\n"
          "}\n"
          ".bar {\n"
          "  display: flex;\n"
          "  align-items: baseline;\n"
          "  justify-content: space-between;\n"
          "  gap: 16px;\n"
          "  padding: 10px 16px;\n"
          "  background: var(--surface-container);\n"
          "  border-bottom: 1px solid var(--outline-variant);\n"
          "}\n"
          ".title {\n"
          "  font: 600 12px/1.4 var(--mono);\n"
          "  letter-spacing: 0.02em;\n"
          "}\n"
          ".session {\n"
          "  color: var(--outline);\n"
          "  font: 11px/1.4 var(--mono);\n"
          "  font-variant-numeric: tabular-nums;\n"
          "}\n"
          "pre.log {\n"
          "  margin: 0;\n"
          "  padding: 4px 0;\n"
          "  background: var(--surface-low);\n"
          "  font: 13px/1.6 var(--mono);\n"
          "  white-space: normal;\n"
          "}\n"
          ".line {\n"
          "  display: grid;\n"
          "  grid-template-columns: 64px 34px minmax(0, 1fr);\n"
          "  gap: 12px;\n"
          "  align-items: start;\n"
          "  padding: 5px 16px;\n"
          "  border-left: 3px solid transparent;\n"
          "  border-bottom: 1px solid rgba(73, 69, 79, 0.35);\n"
          "}\n"
          ".line:last-child { border-bottom: 0; }\n"
          ".line:hover { background: var(--surface-container); }\n"
          ".line-e { border-left-color: var(--error); }\n"
          ".line-w { border-left-color: var(--tertiary); }\n"
          ".line-i { border-left-color: var(--primary); }\n"
          ".line-d { border-left-color: var(--secondary); }\n"
          ".line-v { border-left-color: var(--outline-variant); }\n"
          ".ts {\n"
          "  color: var(--outline);\n"
          "  font-size: 10px;\n"
          "  line-height: 1.6;\n"
          "  white-space: nowrap;\n"
          "  font-variant-numeric: tabular-nums;\n"
          "}\n"
          ".prio {\n"
          "  justify-self: start;\n"
          "  min-width: 22px;\n"
          "  padding: 0 4px;\n"
          "  border-radius: var(--shape-sm);\n"
          "  font: 700 10px/15px var(--mono);\n"
          "  letter-spacing: 0.02em;\n"
          "  text-align: center;\n"
          "}\n"
          ".prio-e { background: var(--error-container); "
          "color: var(--on-error-container); }\n"
          ".prio-w { background: var(--tertiary-container); "
          "color: var(--on-tertiary-container); }\n"
          ".prio-i { background: var(--primary-container); "
          "color: var(--on-primary-container); }\n"
          ".prio-d { background: var(--secondary-container); "
          "color: var(--on-secondary-container); }\n"
          ".prio-v { background: var(--surface-highest); "
          "color: var(--on-surface-variant); }\n"
          ".body {\n"
          "  min-width: 0;\n"
          "  overflow-x: auto;\n"
          "  white-space: pre;\n"
          "  scrollbar-width: thin;\n"
          "  scrollbar-color: var(--outline-variant) transparent;\n"
          "}\n"
          ".line-v .body { color: var(--on-surface-variant); }\n"
          ".loc { color: var(--on-surface-variant); }\n"
          ".loc-file { color: var(--primary); }\n"
          ".loc-func { color: var(--secondary); }\n"
          ".loc-line { color: var(--outline); }\n"
          ".msg-warn { color: var(--tertiary); }\n"
          ".msg-error { color: var(--error); }\n"
          ".image-wrap {\n"
          "  display: block;\n"
          "  overflow-x: auto;\n"
          "  margin: 10px 0 4px;\n"
          "  scrollbar-width: thin;\n"
          "  scrollbar-color: var(--outline-variant) transparent;\n"
          "}\n"
          ".log-image {\n"
          "  display: block;\n"
          "  max-width: none;\n"
          "  height: auto;\n"
          "  margin: 0;\n"
          "  padding: 8px;\n"
          "  background: var(--surface);\n"
          "  border: 1px solid var(--outline-variant);\n"
          "  border-radius: var(--shape-sm);\n"
          "}\n"
          "@media (max-width: 720px) {\n"
          "  .shell {\n"
          "    width: calc(100% - 16px);\n"
          "    margin: 8px auto 16px;\n"
          "  }\n"
          "  .bar { padding: 8px 12px; }\n"
          "  .session { display: none; }\n"
          "  .line {\n"
          "    grid-template-columns: 52px 30px minmax(0, 1fr);\n"
          "    gap: 8px;\n"
          "    padding: 5px 12px;\n"
          "  }\n"
          "  pre.log { font-size: 12px; }\n"
          "  .log-image { padding: 6px; }\n"
          "}\n"
          "</style>\n"
          "</head>\n"
          "<body>\n"
          "<main class=\"shell\">\n"
          "<header class=\"bar\">\n"
          "<span class=\"title\">wst-toys log</span>\n"
          "<span class=\"session\">",
          stream);
    wst_log_html_escape(stream, stamp);
    fputs("</span>\n"
          "</header>\n"
          "<pre class=\"log\">\n",
          stream);
}

int wst_log_html_line(WstLogPrio prio, const char *file, const char *func,
                      int line, const char *body)
{
    const WstLogStyle *style = &wst_log_styles[prio];
    FILE *stream = wst_log_stream();

    size_t body_len = body != NULL ? strlen(body) : 0;
    if (body_len > 0 && body[body_len - 1] == '\n')
        body_len--;

    char clock[WST_LOG_TIMESTAMP_CAP];
    wst_log_timestamp(clock, sizeof(clock), "%H:%M:%S");

    fprintf(stream,
            "<span class=\"line line-%s\">"
            "<span class=\"ts\">%s</span>"
            "<span class=\"prio prio-%s\">%s</span>"
            "<span class=\"body\">",
            style->html_class, clock, style->html_class, style->tag);

    if (file != NULL) {
        fputs("<span class=\"loc\"><span class=\"loc-file\">", stream);
        wst_log_html_escape(stream, file);
        fputs("</span>", stream);
        if (func != NULL) {
            fputs(":<span class=\"loc-func\">", stream);
            wst_log_html_escape(stream, func);
            fputs("</span>", stream);
            if (line > 0)
                fprintf(stream, ":<span class=\"loc-line\">%3d</span>", line);
        }
        fputs("</span>: ", stream);
    }

    fprintf(stream, "<span class=\"%s\">", style->html_body_class);
    html_escape_n(stream, body != NULL ? body : "", body_len);
    fputs("</span></span></span>\n", stream);

    return (int)body_len;
}
