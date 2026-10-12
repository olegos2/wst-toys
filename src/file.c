#include "toys/file.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void wst_file_close(WstFile *file)
{
    if (file == NULL || file->data == NULL)
        return;

    if (file->is_mmap)
        munmap((void *)file->data, file->size);
    else
        free((void *)file->data);

    file->data = NULL;
    file->size = 0;
}

bool wst_file_open(const char *path, WstFile *out)
{
    assert(path != NULL);
    assert(out != NULL);

    wst_file_close(out);

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        LOG_E("cannot open '%s': %s", path, strerror(errno));
        return false;
    }

    struct stat my_stat = { 0 };
    if (fstat(fd, &my_stat) != 0) {
        LOG_E("cannot stat '%s': %s", path, strerror(errno));
        close(fd);
        return false;
    }

    if (!S_ISREG(my_stat.st_mode)) {
        LOG_E("'%s' is not a regular file", path);
        close(fd);
        return false;
    }

    size_t fsize = (size_t)my_stat.st_size;
    if (fsize == 0) {
        LOG_D("'%s' is an empty file", path);
        close(fd);
        return true;
    }

    void *map = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map != MAP_FAILED) {
        LOG_D("'%s': %zu bytes via mmap", path, fsize);
        out->data = (const char *)map;
        out->size = fsize;
        out->is_mmap = true;
        close(fd);
        return true;
    }
    LOG_W("mmap of '%s' failed: %s, falling back to read",
          path, strerror(errno));

    char *buf = calloc(fsize, sizeof(char));
    if (buf == NULL) {
        LOG_E("calloc(%zu) failed: %s", fsize, strerror(errno));
        close(fd);
        return false;
    }

    /* `read` does not guarantee reading all bytes in one go */
    size_t read_so_far = 0;
    while (read_so_far < fsize) {
        ssize_t ret = read(fd, buf + read_so_far, fsize - read_so_far);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            LOG_E("read of '%s' failed: %s", path, strerror(errno));
            free(buf);
            close(fd);
            return false;
        }
        if (ret == 0)
            break; /* EOF */
        read_so_far += (size_t)ret;
    }
    close(fd);

    out->data = buf;
    out->size = read_so_far;
    out->is_mmap = false;
    return true;
}
