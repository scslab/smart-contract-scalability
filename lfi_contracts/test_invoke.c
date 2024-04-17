#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{
    uint8_t* p = malloc(4);
    memcmp(p, "abc\0", 4);
    if (len != 32){
        return -1;
    }
    lfihog_invoke(ptr, 0xAABBCCDD, p, 4);
    return 0;
}
