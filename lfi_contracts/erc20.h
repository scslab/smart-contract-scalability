/**
 * Copyright 2024 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

enum {
    ERC20_CTOR = 0,
	ERC20_MINT = 1,
	ERC20_TRANSFERFROM = 2,
	ERC20_ALLOWANCEDELTA = 3,
	ERC20_BALANCEOF = 4
};

struct calldata_ctor
{
    uint8_t owner[32];
};

struct calldata_transferFrom
{
    uint8_t from[32];
    uint8_t to[32];
    int64_t amount;
};

struct calldata_mint
{
    uint8_t recipient[32];
    int64_t amount;
};

struct calldata_allowanceDelta
{
    uint8_t account[32];
    int64_t amount;
};

struct calldata_balanceOf
{
    uint8_t account[32];
};


void Ierc20_transferFrom(const uint8_t* erc20_addr, const uint8_t* from, const uint8_t* to, int64_t amount)
{
    struct calldata_transferFrom* p = malloc(sizeof(struct calldata_transferFrom));
    memcpy(p->from, from, 32);
    memcpy(p->to, to, 32);
    p->amount = amount;
    lfihog_invoke(erc20_addr, ERC20_TRANSFERFROM, (uint8_t*)p, sizeof(struct calldata_transferFrom), NULL, 0);
    free(p);
}

void Ierc20_allowanceDelta(const uint8_t* erc20_addr, const uint8_t* auth, int64_t amount)
{
    struct calldata_allowanceDelta* p = malloc(sizeof(struct calldata_allowanceDelta));
    memcpy(p->account, auth, 32);
    p->amount = amount;
    lfihog_invoke(erc20_addr, ERC20_ALLOWANCEDELTA, (uint8_t*) p, sizeof(struct calldata_allowanceDelta), NULL, 0);
    free(p);
}

void Ierc20_mint(const uint8_t* erc20_addr, const uint8_t* recipient, int64_t amount)
{
    struct calldata_mint* p = malloc(sizeof(struct calldata_mint));
    memcpy(p->recipient, recipient, 32);
    p->amount = amount;
    lfihog_invoke(erc20_addr, ERC20_MINT, (uint8_t*) p, sizeof(struct calldata_mint), NULL, 0);
    free(p);
}

int64_t Ierc20_balanceOf(const uint8_t* erc20_addr, const uint8_t* addr)
{
    struct calldata_balanceOf* p = malloc(sizeof(struct calldata_balanceOf));
    memcpy(p->account, addr, 32);
    int64_t out;
    int64_t* buf = malloc(8);
    lfihog_invoke(erc20_addr, ERC20_BALANCEOF, (uint8_t*) p, sizeof(struct calldata_balanceOf), (uint8_t*)buf, 8);
    out = *buf;
    free(p);
    free(buf);
    return out;
}

/*
class Ierc20
{
    const sdk::Address& addr;

public:

    constexpr 
    Ierc20(const sdk::Address& addr)
        : addr(addr)
        {}

    inline void transferFrom(
        const sdk::Address& from,
        const sdk::Address& to,
        int64_t amount)
    {
        calldata_transferFrom arg
        {
            .from = from,
            .to = to,
            .amount = amount
        };

        transferFrom(arg);
    }

    inline void transferFrom(
        const calldata_transferFrom& arg)
    {
        sdk::invoke(addr, 1, arg);
    }

    inline void mint(
        const sdk::Address& recipient,
        int64_t amount)
    {
        calldata_mint arg
        {
            .recipient = recipient,
            .amount = amount
        };

        mint(arg);
    }

    inline void mint(
        const calldata_mint& arg)
    {
        sdk::invoke(addr, 2, arg);
    }

    inline void allowanceDelta(
        const sdk::Address& account, 
        int64_t amount)
    {
        calldata_allowanceDelta arg
        {
            .account = account,
            .amount = amount
        };

        allowanceDelta(arg);
    }

    inline void allowanceDelta(
        const calldata_allowanceDelta& arg)
    {
        sdk::invoke(addr, 3, arg);
    }

    inline int64_t balanceOf(
        const sdk::Address& account)
    {
        calldata_balanceOf arg
        {
            .account = account
        };
        return balanceOf(arg);
    }

    inline int64_t balanceOf(
        const calldata_balanceOf& arg)
    {
        return sdk::invoke<calldata_balanceOf, int64_t>(addr, 4, arg);
    }

    inline metadata get_metadata()
    {
        calldata_get_metadata arg;
        return sdk::invoke<calldata_get_metadata, metadata>(addr, 5, arg);
    }
};
*/
