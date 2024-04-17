#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len) {

    uint32_t* p = malloc(sizeof(uint32_t));
    *p = method;
    lfihog_log(p, sizeof(uint32_t));
    lfihog_log(ptr, len);
    return 0;
}
