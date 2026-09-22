
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))


/**
 * A file mapped to memory.
 */
typedef struct {
    const char *data;
    size_t size;
    /**
     * Whether memory is backed by mmap and not malloc'd buffer
     */
    bool is_mmap;
} FileContent;


/** A utf8 line and decoded wchar repr */
typedef struct {
    /* UTF-8 encoded bytes ptr */
    const char *bptr;
    /* UTF-8 encoding length in bytes */
    size_t blen;
    /* wchar decoded str */
    wchar_t *wptr;
    /* wchar decoded str length (chars count) */
    size_t wlen;
} WstLine;

/**
 * Compare `WstLine` wchar strings lowered, skip non-alphabetic chars.
 *
 * @return value below zero if a is less than b, zero if equal,
 *         greater than zero if a is greater than b.
 */
int cmp_letters_fwd(const void *a, const void *b);

/**
 * Compare `WstLine` wchar strings lowered right-to-left, skip non-alphabetic chars.
 *
 * @return value below zero if a is less than b, zero if equal,
 *         greater than zero if a is greater than b.
 */
int cmp_letters_rev(const void *a, const void *b);

/** Decode array of multibyte lines into wchar representations */
bool decode_lines(WstLine *lines, size_t nlines);

/**
 * Split input buffer into headers for each line.
 * Uses '\n' as delim with optional '\r'.
 */
WstLine *split_lines(const char *data, size_t size, size_t *out_len);

/** Free individual lines and lines array itself. */
void free_lines(WstLine *lines, size_t nlines);

/** Write lines that contain alphabetic chars to stream. */
bool write_alpha_lines(FILE *stream, const WstLine *lines, size_t nlines);
