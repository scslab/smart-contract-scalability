#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int cmain() {
    int* p = malloc(10);
    *p = 10;
    printf("hello %p\n", p);

    uint64_t clen = calldata_len();

    printf("calldata_len = %lu\n", clen);

    return 0;
}
