#include "erc20.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>

#include "utils.h"

#include <ed25519.h>

struct calldata_init
{
    uint8_t token[32];
    uint8_t pk[32];
    uint16_t size_increase;
};

struct calldata_transfer
{
    uint8_t to[32];
    int64_t amount;
    uint64_t nonce;
    uint64_t expiration_time;
};

const char* init_semaphore_addr = "init semaphore";
const char* token_addr = "token addr";
const char* pk_addr = "pk addr";
const char* replay_addr = "replay addr";

void toplevel_init(const uint8_t* token, const uint8_t* pk, uint16_t size_increase)
{
	uint8_t* buf = malloc(32);
	memset(buf, 0, 32);

	memcpy(buf, init_semaphore_addr, strlen(init_semaphore_addr));

	semaphore_acquire(buf);

	memset(buf, 0, 32);
	memcpy(buf, token_addr, strlen(token_addr));

	if (lfihog_has_key(buf))
	{
		exit(-1);
	}

	lfihog_raw_mem_set(buf, token, 32);

	memset(buf, 0, 32);
	memcpy(buf, pk_addr, strlen(pk_addr));

	lfihog_raw_mem_set(buf, pk, 32);

	lfihog_self_addr(buf);

	Ierc20_allowanceDelta(token, buf, INT64_MAX);

	memset(buf, 0, 32);
	memcpy(buf, replay_addr, strlen(replay_addr));

	lfihog_hs_inc_limit(buf, size_increase);
}

void
toplevel_transfer(const uint8_t* to, int64_t amount, uint64_t expiration_time)
{
    uint8_t* buf = malloc(32);
    uint8_t* signature = malloc(64);
    uint8_t* pk = malloc(32);
    uint8_t* invoked_hash = malloc(32);
    uint8_t* token = malloc(32);
    uint8_t* self = malloc(32);

    lfihog_invoked_tx_hash(invoked_hash);

    //sdk::record_self_replay(calldata.expiration_time);
    /*
    Hash hash_buf = get_invoked_hash();
	hashset_insert(detail::replay_cache_storage_key, hash_buf, expiration_time);

	uint64_t current_block_number = get_block_number();
	if (expiration_time < current_block_number)
	{
		abort();
	}

	hashset_clear(detail::replay_cache_storage_key, current_block_number);
	*/
    memset(buf, 0, 32);
    memcpy(buf, replay_addr, strlen(replay_addr));
    lfihog_hs_insert(buf, invoked_hash, expiration_time);
    uint64_t cur_block_number = lfihog_block_number();

    if (expiration_time < cur_block_number)
    {
    	exit(-1);
    }
    lfihog_hs_clear(buf, cur_block_number);

    memset(buf, 0, 32);
    memcpy(buf, pk_addr, strlen(pk_addr));
    lfihog_raw_mem_get(buf, pk, 32);

    lfihog_witness_get(0, signature, 64);

    if (ed25519_verify(signature, invoked_hash, 32, pk) != 1)
    {
	printf("sig verify failed!!!!!!!!\n");
    	exit(-1);
    }

    //sdk::auth_single_pk_check_sig(0);
	/*	
	PublicKey pk = get_raw_memory<PublicKey>(detail::pk_storage_key);
	Hash h = get_invoked_hash();
	Signature sig = get_witness<Signature>(wit_idx);

	if (!check_sig_ed25519(pk, sig, h))
	{
		abort();
	} 
	*/

    memset(buf, 0, 32);
    memcpy(buf, token_addr, strlen(token_addr));
    lfihog_raw_mem_get(buf, token, 32);

    lfihog_self_addr(self);

    Ierc20_transferFrom(token, self, to, amount);

    free(self);
    free(token);
    free(invoked_hash);
    free(pk);
    free(signature);
    free(buf);
}



int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{
	switch(method)
	{
	case 0: {
		struct calldata_init* p = (struct calldata_init*)(ptr);
		toplevel_init(p->token, p->pk, p->size_increase);
		return 0;
	}
	case 1: {
		struct calldata_transfer* p = (struct calldata_transfer*)(ptr);
		// nonce unused
		toplevel_transfer(p->to, p->amount, p->expiration_time);
		return 0;
	}


	default:
		return -1;
	}
}
