#ifndef TOYS_STRING_H
#define TOYS_STRING_H

#include <stddef.h>

size_t my_strlen(const char *str);

size_t my_strnlen(const char *str, size_t max_len);

const char *my_strnul(const char *str);

char *my_stpcpy(char *restrict dest, const char *restrict src);

char *my_stpncpy(char *restrict dest, const char *restrict src, size_t dsize);

char *my_strcpy(char *restrict dest, const char *restrict src);

char *my_strncpy(char *restrict dest, const char *restrict src, size_t dsize);

char *my_strdup(const char *str);

int my_memcmp(const void *str1, const void *str2, size_t len);

void *my_memcpy(void *restrict dest, const void *restrict src, size_t len);

int my_strcmp(const char *str1, const char *str2);

char *my_strcat(char *restrict dest, const char *restrict src);

void *my_memchr(const void *str, int c, size_t len);

void *my_memrchr(const void *str, int c, size_t len);

char *my_strchr(const char *str, int c);

char *my_strtok(char *restrict str, const char *restrict delim);

#endif /* TOYS_STRING_H */