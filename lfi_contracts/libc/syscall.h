#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifndef GROUNDHOG_SYSCALL_H
#define GROUNDHOG_SYSCALL_H

void 
lfihog_log(const uint8_t* ptr, uint32_t size);

uint64_t 
lfihog_invoke(const uint8_t* addr /* 32 bytes */, uint32_t method, const uint8_t* calldata, uint32_t calldata_len, uint8_t* return_data, uint32_t return_len);

void
lfihog_sender(uint8_t* buffer /* 32 bytes */);

void lfihog_self_addr(uint8_t* buffer /* 32 bytes */);
void lfihog_src_tx_hash(uint8_t* buffer /* 32 bytes */);
void lfihog_invoked_tx_hash(uint8_t* buffer /* 32 bytes */);

uint64_t lfihog_block_number();

uint8_t lfihog_has_key(const uint8_t* key_buf /* 32 bytes */);

void lfihog_raw_mem_set(const uint8_t* key_buf /* 32 bytes */, uint8_t* mem_offset, uint32_t mem_len);
// returns amount of data written
uint32_t lfihog_raw_mem_get(const uint8_t* key_buf /* 32 bytes*/, uint8_t* buffer_offset, uint8_t max_len);
//returns -1 if key nexist
int32_t lfihog_raw_mem_get_len(const uint8_t* key_buf /* 32 bytes */);

void lfihog_delete_key_last(const uint8_t* key_buf /* 32 bytes */);

void lfihog_nnint_set_add(const uint8_t* key_buf /* 32 bytes */, int64_t set_amount, int64_t delta);
void lfihog_nnint_add(const uint8_t* key_buf /* 32 bytes */, int64_t delta);
int64_t lfihog_nnint_get(const uint8_t* key_buf /* 32 bytes */);

void lfihog_hs_insert(const uint8_t* key_buf /* 32 bytes */, const uint8_t* hash_buf /* 32 bytes */, uint64_t threshold);
void lfihog_hs_inc_limit(const uint8_t* key_buf /* 32 bytes */, uint32_t inc_amount);
void lfihog_hs_clear(const uint8_t* key_buf /* 32 bytes */, uint64_t threshold);
uint32_t lfihog_hs_get_size(const uint8_t* key_buf /* 32 bytes */);
uint32_t lfihog_hs_get_max_size(const uint8_t* key_buf /* 32 bytes */);

/**
 * if two things with same threshold, 
 * returns lowest index.
 * If none, returns -1
 **/
int32_t lfihog_hs_get_index_of(const uint8_t* key_buf /* 32 bytes */, uint64_t threshold);
// returns the associated threshold
uint64_t lfihog_hs_get_index(const uint8_t* key_buf /* 32 bytes */, uint32_t index, uint8_t* output_buffer /* 32 bytes */);

void lfihog_contract_create(uint32_t contract_index, uint8_t* hash_out /* out_len = 32 */);
void lfihog_contract_deploy(const uint8_t* contract_hash /* 32 bytes */, uint64_t nonce, uint8_t* out_addr_offset /* 32 bytes */);

// returns written data amount
uint32_t lfihog_witness_get(uint64_t witness_index, uint8_t* out_offset, uint32_t max_len)
uint32_t lfihog_witness_get_len(uint64_t witness_index);

#endif

