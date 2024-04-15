#include <string.h>

void write(const char*, size_t);
void exit(int);

void _start() {
    write("hello\n", 7);
    exit(0);
}
