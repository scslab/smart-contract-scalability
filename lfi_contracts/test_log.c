#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{

    uint64_t* p = malloc(sizeof(uint64_t));
    *p = 0xAABBCCDDEEFF0011;

    switch(method)
    {
    
    case 0:
    {
        lfihog_log(p, sizeof(uint64_t));
        break;
    }
    case 1:
    {
        lfihog_log(ptr, len);
        break;
    }
    case 2:
    {
        if (len != 8) {
            return -1;
        }
        lfihog_log(ptr, 4);
        lfihog_log(ptr+4, 4);
        break;
    }
    case 3:
    {
        uint8_t* buf = malloc(32);
        lfihog_sender(buf);
        lfihog_log(buf, 32);
        break;
    }
    default:
        return -1;
    }

    return 0;
}
