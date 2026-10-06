#ifndef TOYS_STRING_H
#define TOYS_STRING_H

#include <stddef.h>

/**
 * Calculate the length of a string.
 *
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return The number of bytes in the string, excluding the terminating null byte.
 */
size_t wst_strlen(const char *str);

/**
 * Determine the length of a fixed-size string.
 *
 * @param[in] str Pointer to the string.
 * @param[in] nbytes Maximum number of bytes to examine.
 *
 * @return The number of bytes in the string if less than max_len, otherwise max_len.
 */
size_t wst_strnlen(const char *str, size_t nbytes);

/**
 * Find the terminating null byte of a string.
 *
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return A pointer to the terminating null byte of the string.
 */
const char *wst_strnul(const char *str);

/**
 * Copy a string and return a pointer to its end.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source null-terminated string.
 *
 * @return A pointer to the terminating null byte in the destination buffer.
 */
char *wst_stpcpy(char *restrict dest, const char *restrict src);

/**
 * Copy a fixed-size string and return a pointer to its end.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @param[in] dsize Maximum number of bytes to write.
 *
 * @return A pointer to the destination destination null terminator if src is shorter than dsize, or dest + dsize.
 */
char *wst_stpncpy(char *restrict dest, const char *restrict src, size_t dsize);

/**
 * Copy a string.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source null-terminated string.
 *
 * @return A pointer to the destination buffer dest.
 */
char *wst_strcpy(char *restrict dest, const char *restrict src);

/**
 * Copy a fixed-size string.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @param[in] dsize Number of bytes to write.
 *
 * @return A pointer to the destination buffer dest.
 */
char *wst_strncpy(char *restrict dest, const char *restrict src, size_t dsize);


/**
 * Copy a fixed-size string and NUL-terminate it.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @param[in] dsize Number of bytes to write.
 *
 * @return A pointer to the destination buffer dest.
 */
char *wst_strlcpy(char *restrict dest, const char *restrict src, size_t dsize);

/**
 * Duplicate a string.
 *
 * @param[in] str Pointer to the null-terminated string to duplicate.
 *
 * @return A pointer to the newly allocated string, or NULL if insufficient memory was available.
 */
char *wst_strdup(const char *str);

/**
 * Duplicate a fixed-size string.
 *
 * @param[in] str Pointer to the string to duplicate.
 * @param[in] nbytes Maximum number of bytes to copy.
 *
 * @return A pointer to the newly allocated string, or NULL if insufficient memory was available.
 */
char *wst_strndup(const char *str, size_t nbytes);

/**
 * Compare memory areas.
 *
 * @param[in] str1 Pointer to the first memory area.
 * @param[in] str2 Pointer to the second memory area.
 * @param[in] nbytes Number of bytes to compare.
 *
 * @return An integer less than, equal to, or greater than zero if str1 is found to be less than, match, or be greater than str2.
 */
int wst_memcmp(const void *str1, const void *str2, size_t nbytes);

/**
 * Copy memory area.
 *
 * @param[out] dest Pointer to the destination memory area.
 * @param[in] src Pointer to the source memory area.
 * @param[in] nbytes Number of bytes to copy.
 *
 * @return A pointer to the destination memory area dest.
 */
void *wst_memcpy(void *restrict dest, const void *restrict src, size_t nbytes);

/**
 * Compare two strings.
 *
 * @param[in] str1 Pointer to the first null-terminated string.
 * @param[in] str2 Pointer to the second null-terminated string.
 *
 * @return An integer less than, equal to, or greater than zero if str1 is found to be less than, match, or be greater than str2.
 */
int wst_strcmp(const char *str1, const char *str2);

/**
 * Compare two strings, at most `len` bytes.
 *
 * @param[in] str1 Pointer to the first null-terminated string.
 * @param[in] str2 Pointer to the second null-terminated string.
 * @param[in] nbytes Max number of bytes to compare.
 *
 * @return An integer less than, equal to, or greater than zero if str1 is found to be less than, match, or be greater than str2.
 */
int wst_strncmp(const char *str1, const char *str2, size_t nbytes);

/**
 * Concatenate two strings.
 *
 * @param[in,out] dest Pointer to the destination null-terminated string.
 * @param[in] src Pointer to the source null-terminated string to append.
 *
 * @return A pointer to the destination buffer dest.
 */
char *wst_strcat(char *restrict dest, const char *restrict src);

/**
 * Scan memory for a character.
 *
 * @param[in] str Pointer to the memory area.
 * @param[in] c Character to look for, passed as an int but interpreted as an unsigned char.
 * @param[in] nbytes Number of bytes to scan.
 *
 * @return A pointer to the matching byte, or NULL if the character does not occur in the given memory area.
 */
void *wst_memchr(const void *str, int c, size_t nbytes);

/**
 * Scan memory backwards for a character.
 *
 * @param[in] str Pointer to the memory area.
 * @param[in] c Character to look for, passed as an int but interpreted as an unsigned char.
 * @param[in] nbytes Number of bytes to scan.
 *
 * @return A pointer to the matching byte, or NULL if the character does not occur in the given memory area.
 */
void *wst_memrchr(const void *str, int c, size_t nbytes);

/**
 * Locate character in string.
 *
 * @param[in] str Pointer to the null-terminated string.
 * @param[in] c Character to look for, passed as an int but interpreted as a char.
 *
 * @return A pointer to the first occurrence of the character, or NULL if the character is not found.
 */
char *wst_strchr(const char *str, int c);

/**
 * Locate character in string, searching backwards.
 *
 * @param[in] str Pointer to the null-terminated string.
 * @param[in] c Character to look for, passed as an int but interpreted as a char.
 *
 * @return A pointer to the last occurrence of the character, or NULL if the character is not found.
 */
char *wst_strrchr(const char *str, int c);

/**
 * Extract tokens from strings.
 *
 * @param[in,out] str Pointer to the string to parse, or NULL to continue parsing the previous string.
 * @param[in] delim Pointer to the null-terminated string containing delimiter characters.
 *
 * @return A pointer to the next token, or NULL if no more tokens are found.
 */
char *wst_strtok(char *restrict str, const char *restrict delim);

/**
 * Interchange/swap bytes of 2 memory areas.
 *
 * @param[out] data1 Pointer to the destination memory area.
 * @param[in] data2 Pointer to the source memory area.
 * @param[in] nbytes Number of bytes to swap.
 */
void wst_memswp(void *restrict data1, const void *restrict data2, size_t nbytes);

#endif /* TOYS_STRING_H */