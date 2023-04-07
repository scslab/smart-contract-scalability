/**
 * Copyright 2023 Geoffrey Ramseyer
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

#include "sdk/types.h"
#include "sdk/invoke.h"

namespace erc20
{

struct calldata_transferFrom
{
    sdk::Address from;
    sdk::Address to;
    int64_t amount;
};

struct calldata_mint
{
    sdk::Address recipient;
    int64_t amount;
};


struct calldata_allowanceDelta
{
    sdk::Address account;
    int64_t amount;
};

struct calldata_balanceOf
{
    sdk::Address account;
};

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
        sdk::invoke(addr, 0, arg);
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
        sdk::invoke(addr, 1, arg);
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
        sdk::invoke(addr, 2, arg);
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
        return sdk::invoke<calldata_balanceOf, int64_t>(addr, 3, arg);
    }
};



}