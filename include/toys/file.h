#ifndef TOYS_FILE_H
#define TOYS_FILE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Whole file mapped into memory.
 */
typedef struct {
    /** File bytes, mmap'd or malloc'd. */
    const char *data;
    /** Number of bytes in data. */
    size_t size;
    /**
     * Whether memory is backed by mmap and not malloc'd buffer.
     */
    bool is_mmap;
} WstFile;

/**
 * Map entire file at `path` into memory, malloc + read fallback.
 * Frees previous content. Does not use text mode.
 *
 * @return `true` on success, `false` on failure.
 */
bool wst_file_open(const char *path, WstFile *out);

/**
 * Unmap/free file content buffer, zero the struct.
 * Safe on empty and zero structs.
 */
void wst_file_close(WstFile *file);

#endif /* TOYS_FILE_H */
