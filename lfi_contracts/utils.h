#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>

void semaphore_acquire(const uint8_t* buf)
{
	lfihog_nnint_set_add(buf, 1, -1);
}
