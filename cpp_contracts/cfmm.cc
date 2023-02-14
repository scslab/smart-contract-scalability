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

#include "sdk/alloc.h"
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/general_storage.h"
#include "sdk/delete.h"
#include "sdk/semaphore.h"
#include "sdk/hashset.h"

#include "erc20.h"

namespace cfmm
{

struct config
{
	sdk::Address feeToken;
	sdk::Address tokenA;
	sdk::Address tokenB;
	sdk::Address lpshares;
};

struct CFMMState {
	uint64_t next_round;
	int64_t tokenA_amount;
	int64_t tokenB_amount;
};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
constexpr static sdk::StorageKey state_key = sdk::make_static_key(1, 2);
constexpr static sdk::StorageKey semaphore_key = sdk::make_static_key(2, 2);

bool overflow_int64(uint64_t a, uint64_t b)
{
	unsigned __int128 sum = a;
	sum += b;
	return sum >= INT64_MAX;
}

config
load_config()
{
	return sdk::get_raw_memory<config>(config_addr);
}

void write_config(config const& c)
{
	sdk::set_raw_memory(config_addr, c);
}

CFMMState
load_state()
{
	return sdk::get_raw_memory<CFMMState>(state_key);
}

void write_state(CFMMState const& c)
{
	sdk::set_raw_memory(state_key, c);
}

sdk::StorageKey 
get_hs_key_for_round(uint64_t round_number)
{
	return sdk::hash(round_number);
}

struct swap
{
	sdk::Address seller;
	bool direction; // true: A to B, false: B to A
	int64_t sellAmount;
	int64_t minReceive;
};

struct deposit
{
	sdk::Address depositor;
	int64_t depositA;
	int64_t depositB;
};

struct withdraw
{
	sdk::Address withdrawer;
	int64_t lp_shares;
};

struct event {
	uint8_t type;
	int64_t fee_bid;
	uint64_t round;
	union {
		swap s;
		deposit d;
		withdraw w;
	} obj;
};

void add_swap(swap const& s, int64_t fee_bid)
{
	auto c = load_config();
	auto key = get_hs_key_for_round(sdk::get_block_number());

	event e;
	e.obj.s = s;
	e.type = 0;
	e.fee_bid = fee_bid;
	e.round = sdk::get_block_number();

	auto h = sdk::hash(e);

	sdk::hashset_insert(key, h, e.fee_bid);

	sdk::set_raw_memory(h, e);

	erc20::Ierc20 fee_token (c.feeToken);

	fee_token.transferFrom(sdk::get_msg_sender(), sdk::get_self(), e.fee_bid);

	if (s.direction)
	{
		erc20::Ierc20 sell_token(c.tokenA);
		sell_token.transferFrom(s.seller, sdk::get_self(), s.sellAmount);
	}
	else
	{
		erc20::Ierc20 sell_token(c.tokenB);
		sell_token.transferFrom(s.seller, sdk::get_self(), s.sellAmount);
	}
}

void add_deposit(deposit const& d, int64_t fee_bid)
{
	auto c = load_config();
	auto key = get_hs_key_for_round(sdk::get_block_number());

	event e;
	e.obj.d = d;
	e.type = 1;
	e.fee_bid = fee_bid;
	e.round = sdk::get_block_number();

	auto h = sdk::hash(e);

	sdk::hashset_insert(key, h, e.fee_bid);

	sdk::set_raw_memory(h, e);

	erc20::Ierc20 fee_token (c.feeToken);

	fee_token.transferFrom(sdk::get_msg_sender(), sdk::get_self(), e.fee_bid);

	erc20::Ierc20 tokenA (c.tokenA);
	tokenA.transferFrom(d.depositor, sdk::get_self(), d.depositA);

	erc20::Ierc20 tokenB (c.tokenB);
	tokenA.transferFrom(d.depositor, sdk::get_self(), d.depositB);
}

void add_withdraw(withdraw const& w, int64_t fee_bid)
{
	auto c = load_config();
	auto key = get_hs_key_for_round(sdk::get_block_number());

	event e;
	e.obj.w = w;
	e.type = 2;
	e.fee_bid = fee_bid;
	e.round = sdk::get_block_number();

	auto h = sdk::hash(e);

	sdk::hashset_insert(key, h, e.fee_bid);

	sdk::set_raw_memory(h, e);

	erc20::Ierc20 fee_token (c.feeToken);

	fee_token.transferFrom(sdk::get_msg_sender(), sdk::get_self(), e.fee_bid);

	erc20::Ierc20 lptoken (c.lpshares);
	lptoken.transferFrom(w.withdrawer, sdk::get_self(), w.lp_shares);
}

void refund_swap(const config& config, swap const& swap)
{
	if (swap.direction)
	{
		erc20::Ierc20 selltoken (config.tokenA);
		selltoken.transferFrom(sdk::get_self(), swap.seller, swap.sellAmount);
	}
	else
	{
		erc20::Ierc20 selltoken (config.tokenB);
		selltoken.transferFrom(sdk::get_self(), swap.seller, swap.sellAmount);
	}
}

void refund_deposit(const config& config, deposit const& d)
{
	erc20::Ierc20 tokenA (config.tokenA);
	tokenA.transferFrom(sdk::get_self(), d.depositor, d.depositA);

	erc20::Ierc20 tokenB (config.tokenB);
	tokenA.transferFrom( sdk::get_self(), d.depositor, d.depositB);
}

void refund_withdraw(const config& config, withdraw const& w)
{
	erc20::Ierc20 lptoken (config.lpshares);
	lptoken.transferFrom(sdk::get_self(), w.withdrawer, w.lp_shares);
}

void exec_one_swap(const config& config, CFMMState& state, swap const& swap)
{
	unsigned __int128 prod = state.tokenA_amount * state.tokenB_amount;

	sdk::Address const& buy_token_addr = swap.direction ? config.tokenB : config.tokenA;

	int64_t initial_selltoken = swap.direction ? state.tokenA_amount : state.tokenB_amount;
	int64_t initial_buytoken = swap.direction ? state.tokenB_amount : state.tokenA_amount;

	if (overflow_int64(initial_selltoken, swap.sellAmount))
	{
		refund_swap(config, swap);
		return;
	}

	int64_t new_sell = initial_selltoken + swap.sellAmount;
	int64_t new_buy = prod / new_sell;
	if (((unsigned __int128) new_buy) * ((unsigned __int128) new_sell) < prod)
	{
		// rounding in favor of cfmm
		new_buy++;
	}

	int64_t payout = initial_buytoken - new_buy; // nonnegative

	if (payout >= swap.minReceive)
	{
		erc20::Ierc20 buytoken(buy_token_addr);
		buytoken.transferFrom(sdk::get_self(), swap.seller, payout);

		state.tokenA_amount = swap.direction ? new_sell : new_buy;
		state.tokenB_amount = swap.direction ? new_buy : new_sell;
		return;
	} 
	else
	{
		refund_swap(config, swap);
		return;
	}
}

std::pair<uint64_t, uint64_t> babylonian_sqrt(unsigned __int128 prod)
{
	uint64_t r1 = 2;
	uint64_t r2 = prod/r1;

	while (std::max(r1, r2) - std::min(r1, r2) > 1)
	{
		r2 = (r1 + r2) / 2;
		r1 = prod / r2;
	}
	return {r1, r2};
}

uint64_t ub(std::pair<uint64_t, uint64_t> val)
{
	return std::max(val.first, val.second);
}
uint64_t lb(std::pair<uint64_t, uint64_t> val)
{
	return std::min(val.first, val.second);
}

void exec_one_deposit(const config& config, CFMMState& state, deposit const& deposit)
{
	unsigned __int128 prod = state.tokenA_amount * state.tokenB_amount;
	uint64_t old_liquidity_ub = ub(babylonian_sqrt(prod));

	if (overflow_int64(state.tokenA_amount, deposit.depositA))
	{
		refund_deposit(config, deposit);
		return;
	}
	if (overflow_int64(state.tokenB_amount, deposit.depositB))
	{
		refund_deposit(config, deposit);
		return;
	}

	unsigned __int128 prod2 = (state.tokenA_amount + deposit.depositA) * (state.tokenB_amount + deposit.depositB);
	uint64_t new_liquidity_lb = lb(babylonian_sqrt(prod2));

	uint64_t difference = new_liquidity_lb - old_liquidity_ub;

	if (new_liquidity_lb <= old_liquidity_ub)
	{
		refund_deposit(config, deposit);
		return;
	}

	if (difference > INT64_MAX)
	{
		refund_deposit(config, deposit);
		return;
	}


	erc20::Ierc20 lp_token(config.lpshares);

	lp_token.mint(deposit.depositor, difference);

	state.tokenA_amount += deposit.depositA;
	state.tokenB_amount += deposit.depositB;
}

void exec_one_withdraw(const config& config, CFMMState& state, withdraw const& withdraw)
{
	unsigned __int128 prod = state.tokenA_amount * state.tokenB_amount;
	uint64_t old_liquidity_ub = ub(babylonian_sqrt(prod));

	if (withdraw.lp_shares > old_liquidity_ub)
	{
		refund_withdraw(config, withdraw);
		return;
	}

	using uint128 = unsigned __int128;

	//round against withdrawer
	int64_t withdrawA = (((uint128) state.tokenA_amount) * ((uint128) withdraw.lp_shares)) / old_liquidity_ub;
	int64_t withdrawB = (((uint128) state.tokenB_amount) * ((uint128) withdraw.lp_shares)) / old_liquidity_ub;

	erc20::Ierc20 tokenA(config.tokenA);
	tokenA.transferFrom(sdk::get_self(), withdraw.withdrawer, withdrawA);

	erc20::Ierc20 tokenB(config.tokenB);
	tokenB.transferFrom(sdk::get_self(), withdraw.withdrawer, withdrawB);
}

void exec_round(const config& config, uint64_t round)
{
	auto state = load_state();

	sdk::Semaphore semaphore(semaphore_key);
	semaphore.acquire();

	if (state.next_round  != round)
	{
		abort();
	}
	state.next_round++;

	auto hs_key = get_hs_key_for_round(round);

	uint16_t offer_cnt = sdk::hashset_get_size(hs_key);

	erc20::Ierc20 fee_token(config.feeToken);
	int64_t fee_acc = 0;

	for (size_t i = 0; i < offer_cnt; i++)
	{
		auto [fee, event_hash] = sdk::hashset_get_index(hs_key, i);

		auto event = sdk::get_raw_memory<struct event>(event_hash);
		sdk::delete_last(event_hash);

		if (event.type == 0)
		{
			exec_one_swap(config, state, event.obj.s);
		} 
		else if (event.type == 1)
		{
			exec_one_deposit(config, state, event.obj.d);
		}
		else if (event.type == 2)
		{
			exec_one_withdraw(config, state, event.obj.w);
		}
		else {
			// impossible
			abort();
		}

		fee_acc += fee; // assumes fee token has issuance cap of int64_max
		// otherwise we cannot store that much fee token in contract's balance anyways
	}
	fee_token.transferFrom(sdk::get_self(), sdk::get_msg_sender(), fee_acc);

	write_state(state);

	sdk::delete_last(hs_key);
}

EXPORT("pub00000000")
initialize()
{
	sdk::Semaphore s(semaphore_key);
	s.acquire();

	auto calldata = sdk::get_calldata<config>();
	write_config(calldata);

	CFMMState state;
	write_state(state);
}

struct swap_call
{
	bool direction; // true: A to B, false: B to A
	int64_t sellAmount;
	int64_t minReceive;
	int64_t fee_bid;
};

EXPORT("pub01000000")
swap()
{
	auto calldata = sdk::get_calldata<swap_call>();

	struct swap s {
		.seller = sdk::get_msg_sender(),
		.direction = calldata.direction,
		.sellAmount = calldata.sellAmount,
		.minReceive = calldata.minReceive
	};
	add_swap(s, calldata.fee_bid);
}

struct deposit_call
{
	int64_t depositA;
	int64_t depositB;
	int64_t fee_bid;
};

EXPORT("pub02000000")
deposit()
{
	auto calldata = sdk::get_calldata<deposit_call>();

	struct deposit d {
		.depositor = sdk::get_msg_sender(),
		.depositA = calldata.depositA,
		.depositB = calldata.depositB,
	};
	add_deposit(d, calldata.fee_bid);
}

struct withdraw_call
{
	int64_t lpshares;
	int64_t fee_bid;
};

EXPORT("pub03000000")
withdraw()
{
	auto calldata = sdk::get_calldata<withdraw_call>();

	struct withdraw w {
		.withdrawer = sdk::get_msg_sender(),
		.lp_shares = calldata.lpshares,
	};
	add_withdraw(w, calldata.fee_bid);
}

struct round_bounds {
	uint64_t start_round;
	uint64_t end_round;
};

EXPORT("pub04000000")
execute()
{
	auto calldata = sdk::get_calldata<round_bounds>();

	if (calldata.end_round >= sdk::get_block_number())
	{
		abort();
	}

	auto config = load_config();

	for (uint64_t i = calldata.start_round; i <= calldata.end_round; i++)
	{
		exec_round(config, i);
	}
}










}