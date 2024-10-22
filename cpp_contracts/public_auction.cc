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

#include "erc20.h"
#include "erc721.h"

#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/auth_singlekey.h"
#include "sdk/log.h"
#include "sdk/flag.h"
#include "sdk/witness.h"
#include "sdk/crypto.h"

#include <cstdint>
#include <algorithm>

namespace public_auction
{

/**
 * 
 * Design: maintain a list of bids, and a list of refunded bids.
 * Alternative: refund bids in batches (and use hashset_clear).
 *   Downside of this is that nobody can free their money if 
 *   one transfer fails (i.e. if the erc20 token starts
 *   rejecting transfers to a certain address).
 */

struct auction_static_config
{
	sdk::Address addrSeller;
	sdk::Address erc721Address;
	sdk::Hash erc721Object;

	// buy token
	sdk::Address token;
	uint64_t closing_round;
};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);

// reserve price not strictly necessary, but useful if we wanted to generalize to kth price
constexpr static sdk::StorageKey reserve_price_addr = sdk::make_static_key(1, 2);
constexpr static sdk::StorageKey auction_hashset = sdk::make_static_key(2, 2);
constexpr static sdk::StorageKey refunded_bids_hashset = sdk::make_static_key(3, 2);

constexpr static sdk::StorageKey creation_semaphore = sdk::make_static_key(4, 2);

struct bid
{
	sdk::Address bidder;
	int64_t amount;
};

void
submit_bid(bid const& b, auction_static_config const& config)
{
	if (sdk::get_block_number() > config.closing_round)
	{
		abort();
	}

	sdk::Hash h = sdk::hash(b);

	int64_t reserve_price = sdk::int64_get(reserve_price_addr);

	if (reserve_price >= b.amount)
	{
		abort();
	}

	if (b.amount < 0)
	{
		abort();
	}

	// 0 <= b.amount <= INT64_MAX

	if (sdk::hashset_get_size(auction_hashset) >= 2)
	{
		// therefore all elts inside hashset are bounded by INT64_MAX, ok to convert
		int64_t new_reserve = static_cast<int64_t>(sdk::hashset_get_index(auction_hashset, 1).first);
		if (new_reserve != reserve_price)
		{
			sdk::int64_set_add(reserve_price_addr, new_reserve, 0);
		}
	}

	erc20::Ierc20 token(config.token);
	token.transferFrom(b.bidder, sdk::get_self(), b.amount);

	sdk::hashset_insert(auction_hashset, h, b.amount);
}

sdk::Hash get_winning_hash()
{
	return sdk::hashset_get_index(auction_hashset, 0).second;
}


void refund_bid(bid const& b, auction_static_config const& config)
{
	uint64_t current_block = sdk::get_block_number();

	if (current_block <= config.closing_round)
	{
		abort();
	}

	sdk::Hash h = sdk::hash(b);

	auto winning_hash = get_winning_hash();

	if (h == winning_hash) {
		abort();
	}

	uint32_t idx = sdk::hashset_get_index_of(auction_hashset, b.amount);

	// assert bid is in set of previous bids
	while(true)
	{
		auto [amt, hash] = sdk::hashset_get_index(auction_hashset, idx);
		if (hash == h)
		{
			break;
		}
		idx++;
	}

	// assert bid is not in set of refunded bids
	sdk::hashset_insert(refunded_bids_hashset, h, b.amount);

	erc20::Ierc20 token(config.token);
	token.transferFrom(sdk::get_self(), b.bidder, b.amount);
}

void execute_winning_bid(bid const& b, auction_static_config const& config)
{
	uint64_t current_block = sdk::get_block_number();

	if (current_block <= config.closing_round)
	{
		abort();
	}

	sdk::Hash h = sdk::hash(b);

	auto winning_hash = get_winning_hash();

	if (winning_hash != h)
	{
		abort();
	}

	int64_t reserve_price = sdk::int64_get(reserve_price_addr);

	if (sdk::hashset_get_size(auction_hashset) > 1)
	{
		auto [limit_bid, _] = sdk::hashset_get_index(auction_hashset, 1);

		reserve_price = std::max(static_cast<int64_t>(limit_bid), reserve_price);
	}

	int64_t refund_amount = b.amount - reserve_price;

	erc20::Ierc20 token(config.token);

	erc721::Ierc721 auctioned_object(config.erc721Address);

	token.transferFrom(sdk::get_self(), b.bidder, refund_amount);
	token.transferFrom(sdk::get_self(), config.addrSeller, reserve_price);

	auctioned_object.transferFrom(sdk::get_self(), b.bidder, config.erc721Object);
}

void destroy_empty_auction(auction_static_config const& config)
{
	if (sdk::hashset_get_size(auction_hashset) != 0)
	{
		abort();
	}

	if (sdk::get_block_number() <= config.closing_round)
	{
		abort();
	}

	erc721::Ierc721 auctioned_object(config.erc721Address);

	auctioned_object.transferFrom(sdk::get_self(), config.addrSeller, config.erc721Object);
}

void increase_auction_sizes(uint16_t limit)
{
	sdk::hashset_increase_limit(auction_hashset, limit);
	sdk::hashset_increase_limit(refunded_bids_hashset, limit);
}

void write_config(auction_static_config const& config)
{
	sdk::set_raw_memory(config_addr, config);
}

auction_static_config
get_config()
{
	return sdk::get_raw_memory<auction_static_config>(config_addr);
}

void create_auction(auction_static_config const& config)
{
	sdk::TransientSemaphore s(creation_semaphore);
	s.acquire();

	erc721::Ierc721 auctioned_object(config.erc721Address);

	auctioned_object.transferFrom(config.addrSeller, sdk::get_self(), config.erc721Object);

	write_config(config);
}

EXPORT("pub00000000")
initialize()
{
	auto calldata = sdk::get_calldata<auction_static_config>();

	create_auction(calldata);
	return 0;
}

struct calldata_submit_bid {
	int64_t amount;
};

EXPORT("pub01000000")
pub_submit_bid()
{
	auto calldata = sdk::get_calldata<calldata_submit_bid>();

	bid b {
		.bidder = sdk::get_msg_sender(),
		.amount = calldata.amount
	};

	submit_bid(b, get_config());
	return 0;
}

EXPORT("pub02000000")
pub_refund_bid()
{
	auto calldata = sdk::get_calldata<bid>();

	refund_bid(calldata, get_config());
	return 0;
}

EXPORT("pub03000000")
pub_winning_bid()
{
	auto calldata = sdk::get_calldata<bid>();

	execute_winning_bid(calldata, get_config());
	return 0;
}

EXPORT("pub04000000")
pub_destroy_auction()
{
	destroy_empty_auction(get_config());
	return 0;
}


}
