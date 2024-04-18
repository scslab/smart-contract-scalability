#include "erc20.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>

#include "utils.h"

#include <sodium.h>


void calculate_balance_key(const uint8_t* addr, uint8_t* addr_out)
{
	if (crypto_generichash(addr_out, 32,
				addr, 32,
				NULL, 0) != 0)
	{
		exit(-1);
	}
}

uint8_t allowance_key_buf[64];

void calculate_allowance_key(const uint8_t* owner,
		const uint8_t* auth,
		uint8_t* addr_out)
{
	std::memcpy(allowance_key_buf, owner, 32);
	std::memcpy(allowance_key_buf + 32, auth, 32);
	if (crypto_generichash(addr_out, 32,
				allowance_key_buf, 64,
				NULL, 0) != 0)
	{
		exit(-1);
	}
}

void transfer(const uint8_t* from, const uint8_t* to, const int64_t amount)
{
	if (amount < 0) {
		exit(-1);
	}

	uint8_t buf[32];
	calculate_balance_key(from, buf);
	lfihog_nnint_add(buf, -amount);

	calculate_balance_key(to, buf);
	lfihog_nnint_add(buf, amount);
}

void allowance_delta(const uint8_t* owner, const uint8_t* auth, const int64_t amount)
{
	uint8_t buf[32];
	calculate_allowance_key(owner, auth, buf);
	lfihog_nnint_add(buf, amount);
}

void toplevel_transferFrom(const uint8_t* owner, const uint8_t* to, const int64_t amount)
{
	uint8_t* sender_buf = malloc(32);
	lfihog_sender(sender_buf);
	allowance_delta(owner, sender, -amount);
	transfer(owner, to, amount);
    free(sender_buf);
}

void
toplevel_mint(const uint8_t* to, const int64_t amount)
{
    if (amount < 0) {
        exit(-1);
    }
    uint8_t buf[32];
    calculate_balance_key(to, buf);
    lfihog_nnint_add(buf, amount);

    memset(buf, 0, 32);

    const char* total_supply_storage_key = "total supply storage key";

    memcpy(buf, total_supply_storage_key, strlen(total_supply_storage_key));
    lfihog_nnint_add(buf, amount);
}

void toplevel_ctor(const uint8_t* owner)
{
    uint8_t buf[32];
    const char* owner_key = "owner key";
    memcpy(buf, owner_key, strlen(owner_key));

    lfihog_raw_mem_set(buf, owner, 32);
}

void toplevel_allowancedelta(const uint8_t* authorized, int64_t amount)
{
    uint8_t* sender_buf = malloc(32);
    lfihog_sender(sender_buf);

    allowance_delta(sender_buf, authorized, amount);

    free(sender_buf);
}

void toplevel_balanceOf(const uint8_t* addr)
{
    uint8_t buf[32];
    calculate_balance_key(addr, buf);

    int64_t* balance = malloc(sizeof(int64_t));
    *balance = lfihog_nnint_get(buf);

    lfihog_return((uint8_t*)(balance), sizeof(int64_t));
}

int cmain(uint32_t method, uint8_t* ptr, uint32_t len)
{
	if (sodium_init() == -1) {
		return -1;
	}

	switch(method)
	{
        case ERC20_CTOR:
        {
            struct calldata_ctor* p = (struct calldata_ctor*)(ptr);
            toplevel_ctor(p->owner);
            return 0;
        }
		case ERC20_TRANSFERFROM:
		{
			struct calldata_transferFrom* p = (struct calldata_transferFrom*)(ptr);
			toplevel_transferFrom(p->from, p->to, p->amount);
			return 0;
		}
        case ERC20_MINT:
        {
            struct calldata_mint* p = (struct calldata_mint*)(ptr);
            toplevel_mint(p->recipient, p -> amount);
            return 0;
        }
        case ERC20_ALLOWANCEDELTA:
        {
            struct calldata_allowanceDelta* p = (struct calldata_allowanceDelta*) (ptr);
            toplevel_allowancedelta(p->account, p->amount);
            return 0;
        }
        case ERC20_BALANCEOF:
        {
            struct calldata_balanceOf* p = (struct calldata_balanceOf*) (ptr);
            toplevel_balanceOf(p->account);
            return 0;
        }
		default:
			// unknown method
			return -1;
	}
}



