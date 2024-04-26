#include <string.h>
#include <stdint.h>

int cmain(uint64_t, uint64_t*, size_t);
void exit(int);

void _start(uint64_t func, uint64_t* args, size_t nargs) {
    int r = cmain(func, args, nargs);
    exit(r);
}
