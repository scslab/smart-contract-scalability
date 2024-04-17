#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifndef GROUNDHOG_SYSCALL_H
#define GROUNDHOG_SYSCALL_H

void 
lfihog_log(uint8_t* ptr, uint32_t size);

uint64_t 
lfihog_invoke(uint8_t* addr /* 32 bytes */, uint32_t method, uint8_t* calldata, uint32_t calldata_len, uint8_t* return_data, uint32_t return_len);

void
lfihog_sender(uint8_t* buffer /* 32 bytes */);

void lfihog_self_addr(uint8_t* buffer /* 32 bytes */);
void lfihog_src_tx_hash(uint8_t* buffer /* 32 bytes */);
void lfihog_invoked_tx_hash(uint8_t* buffer /* 32 bytes */);

#endif

