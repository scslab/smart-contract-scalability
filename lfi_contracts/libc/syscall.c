#include <string.h>
#include <errno.h>
#include <stdint.h>

int _write(int, void*, size_t);

int write(int fd, void* buf, size_t size) {
    int n = _write(fd, buf, size);
    return n;
}

void _exit(int);

