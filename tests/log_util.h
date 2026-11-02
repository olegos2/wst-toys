#ifndef TESTS_LOG_UTIL_H
#define TESTS_LOG_UTIL_H

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *test_path_join(const char *dir, const char *name)
{
    size_t len = strlen(dir) + strlen(name) + 2;
    char *path = malloc(len);
    if (path == NULL)
        return NULL;
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}

static char *test_tmpdir(const char *prefix)
{
    const char *base = getenv("TMPDIR");
    if (base == NULL || base[0] == '\0')
        base = "/tmp";

    size_t len = strlen(base) + strlen(prefix) + 16;
    char *path = malloc(len);
    if (path == NULL)
        return NULL;

    snprintf(path, len, "%s/%s-XXXXXX", base, prefix);
    if (mkdtemp(path) == NULL) {
        free(path);
        return NULL;
    }
    return path;
}

static void test_rm_rf(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (dir != NULL) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 ||
                    strcmp(entry->d_name, "..") == 0)
                    continue;

                char *child = test_path_join(path, entry->d_name);
                if (child != NULL) {
                    test_rm_rf(child);
                    free(child);
                }
            }
            closedir(dir);
        }
        rmdir(path);
    } else {
        unlink(path);
    }
}

static char *test_read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *data = malloc((size_t)size + 1);
    if (data == NULL) {
        fclose(file);
        return NULL;
    }

    size_t got = fread(data, 1, (size_t)size, file);
    fclose(file);
    data[got] = '\0';
    return data;
}

static bool test_endswith(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);
    return text_len >= suffix_len &&
           strcmp(text + text_len - suffix_len, suffix) == 0;
}

#endif /* TESTS_LOG_UTIL_H */
