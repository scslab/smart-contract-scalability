#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>

struct calldata_DeployAndInitialize
{
	uint8_t contract_hash[32];
	uint64_t nonce;
	uint32_t ctor_method;
	uint32_t ctor_calldata_size;
};

struct calldata_Deploy
{
	uint8_t contract_hash[32];
	uint64_t nonce;
};

struct calldata_Create
{
	uint32_t contract_idx;
};



int cmain(uint32_t method, uint8_t* ptr, uint32_t len) {
	switch(method)
	{
	case 0:
	{
		calldata_DeployAndInitialize* c = reinterpret_cast<calldata_DeployAndInitialize*>(ptr);

		uint8_t* deploy_addr = malloc(32);

		lfihog_contract_deploy(&(c->contract_hash[0]), c->nonce, deploy_addr);

		uint8_t* child_calldata = ptr + sizeof(calldata_DeployAndInitialize);
		uint32_t child_calldata_len = len - sizeof(calldata_DeployAndInitialize);

		lfihog_invoke(deploy_addr, c->ctor_method, child_calldata, child_calldata_len, NULL, 0);
		break;
	}
	case 1:{
		calldata_Deploy* c = reinterpret_cast<calldata_Deploy*>(ptr);

		uint8_t* deploy_addr = malloc(32);

		lfihog_contract_deploy(&(c->contract_hash[0]), c->nonce, deploy_addr);
		break;
	}
	case 2:{
		calldata_Create* c = reinterpret_cast<calldata_Create*>(ptr);
		lfihog_contract_create(c->contract_idx, NULL);
		break;
	}
	default:
		return -1;
	}
	return 0;
}
