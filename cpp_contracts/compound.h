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

namespace compound
{

constexpr static uint8_t MAX_ASSETS = 15;

struct interest_rate_params
{
    //measured as 1 << 48
    uint64_t supplyRateBase;
    uint64_t supplyRateLow;
    uint64_t supplyRateHigh;
    //measured as 1 << 32
    uint64_t supplyKink;

    // measured as 1 << 48
    uint64_t borrowRateBase;
    uint64_t borrowRateLow;
    uint64_t borrowRateHigh;
    // measured as 1 << 32
    uint64_t borrowKink;
};

struct calldata_ctor
{
    sdk::Address base_token_addr;
    sdk::Address administrator;
};

struct calldata_admin
{
    uint64_t reserve_target;
    uint8_t storefront_discount;

    interest_rate_params interest;
};

struct calldata_list_asset
{
    sdk::Address addr;
};

struct calldata_update_asset_params
{
    uint8_t asset;
    uint8_t borrow_frac;
    uint8_t liquidity_sell_frac;
    uint8_t liquidity_collateral_frac;
};

struct calldata_update_prices
{
    std::array<uint32_t, MAX_ASSETS> new_prices;
};

struct calldata_adjust_supply_cap
{
    uint8_t asset;
    int64_t amount;
};

struct calldata_transfer_asset
{
    uint8_t asset;
    int64_t amount;
    sdk::Address to;
};

struct calldata_transfer_base
{
    int64_t amount_from_borrow;
    int64_t amount_from_supply;
    sdk::Address to;
};

struct calldata_withdraw_asset
{
    uint8_t asset;
    int64_t amount;
    sdk::Address to;
};

struct calldata_withdraw_base
{
    int64_t amount_from_borrow;
    int64_t amount_from_supply;
    sdk::Address to;
};

struct calldata_supply_asset
{
    uint8_t asset;
    int64_t amount;
    sdk::Address to;
};

struct calldata_supply_base
{
    int64_t amount_from_borrow;
    int64_t amount_from_supply;
    sdk::Address to;
};

struct calldata_normalize_account
{
    int64_t amount; // amount from borrow to supply
};

struct calldata_absorb
{
    sdk::Address account;
};

struct calldata_buy_collateral
{
    uint8_t asset;
    uint64_t base_amount;
    uint64_t min_recv_amt;
};





}
