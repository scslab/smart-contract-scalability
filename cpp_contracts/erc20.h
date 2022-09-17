#pragma once

#include "sdk/types.h"

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

class Ierc20
{
    const sdk::Address addr;

public:

    constexpr 
    Ierc20(const sdk::Address& addr)
        : addr(addr)
        {}

    constexpr
    Ierc20(sdk::Address&& addr)
        : addr(std::move(addr))
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
};



}