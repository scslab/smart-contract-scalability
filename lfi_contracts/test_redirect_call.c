#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{
	if (len < 36) {
		return -1;
	}

	lfihog_invoke(ptr,
		*((uint32_t*)(ptr + 32)), len - 36);
}

