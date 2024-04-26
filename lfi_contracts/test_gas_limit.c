#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{
    uint64_t* p = malloc(sizeof(uint64_t));
    *p = 1;

    switch(method)
    {
    
    case 0:
    {
	uint64_t loops = *((uint64_t*)(ptr));
	for (uint64_t i = 0; i < loops; i++)
	{
		lfihog_log((const uint8_t*)p, sizeof(uint64_t));
	}
        break;
    }
    case 1:
    {
	while(1)
	{
        	lfihog_log((const uint8_t*)p, sizeof(uint64_t));
	}
    }
    default:
        return -1;
    }

    return 0;
}
