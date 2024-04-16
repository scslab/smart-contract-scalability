#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifndef GROUNDHOG_SYSCALL_H
#define GROUNDHOG_SYSCALL_H

// returns method name
uint32_t calldata(void* offset);
uint64_t calldata_len();

#endif

