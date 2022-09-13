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

}