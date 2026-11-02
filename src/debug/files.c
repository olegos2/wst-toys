#include "debug_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void wst_log_timestamp(char *out, size_t cap, const char *fmt)
{
    time_t now = time(NULL);
    struct tm local;
    if (now == (time_t)-1 || localtime_r(&now, &local) == NULL) {
        snprintf(out, cap, "unknown");
        return;
    }
    if (strftime(out, cap, fmt, &local) == 0)
        snprintf(out, cap, "unknown");
}

char *wst_log_path_join(const char *dir, const char *name, const char *ext)
{
    if (dir == NULL || name == NULL || ext == NULL)
        return NULL;

    size_t len = strlen(dir) + strlen(name) + strlen(ext) + 2;
    char *path = malloc(len);
    if (path == NULL)
        return NULL;
    snprintf(path, len, "%s%s%s%s", dir, dir[0] != '\0' ? "/" : "", name,
             ext);
    return path;
}

char *wst_log_sanitize_name(const char *name)
{
    if (name == NULL || name[0] == '\0')
        name = "graph";

    size_t len = strlen(name);
    if (len > 80)
        len = 80;

    char *out = malloc(len + 1);
    if (out == NULL)
        return NULL;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (isalnum(c) || c == '.' || c == '_' || c == '-')
            out[i] = (char)c;
        else
            out[i] = '_';
    }
    out[len] = '\0';
    return out;
}
