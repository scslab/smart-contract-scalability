#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len) {
    int* p = malloc(10);
    *p = 10;
    printf("hello %p\n", p);

    printf("calldata = %s len %u\n", ptr, len);

    return 0;
}
