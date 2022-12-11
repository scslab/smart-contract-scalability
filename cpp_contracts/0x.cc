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

#include "erc20.h"

namespace zerox
{

using sdk::StorageKey;

static std::array<uint8_t, 33> key_buffer;

StorageKey get_balance_key(sdk::Hash const& offer)
{
	std::memcpy(key_buffer.data(), offer.data(), offer.size());
	key_buffer[32] = 0;
	StorageKey out;
	sdk::hash(key_buffer, out);
	return out;
}

StorageKey get_config_key(sdk::Hash const& offer)
{
	std::memcpy(key_buffer.data(), offer.data(), offer.size());
	key_buffer[32] = 1;
	StorageKey out;
	sdk::hash(key_buffer, out);
	return out;
}

StorageKey get_transient_semaphore_key(sdk::Hash const& offer)
{
	std::memcpy(key_buffer.data(), offer.data(), offer.size());
	key_buffer[32] = 2;
	StorageKey out;
	sdk::hash(key_buffer, out);
	return out;
}

struct offer_config {
	sdk::Address owner;
	sdk::Address sellToken;
	sdk::Address buyToken;
	int64_t sellAmount;
	uint64_t price;  // semantic value = price / 2^32
};

int64_t get_buy_amount(int64_t sell_amount, uint64_t price)
{
	__int128 prod = sell_amount * price;
	int64_t round_up = 0;
	if (prod & 0xFFFF'FFFF)
	{
		round_up = 1;
	}
	prod >>= 32;
	if (prod > INT64_MAX)
	{
		abort();
	}
	return prod + round_up;
}

void create_offer(offer_config const& c)
{
	sdk::Hash offer_hash = sdk::hash(c);

	sdk::TransientSemaphore<> s(get_transient_semaphore_key(offer_hash));
	s.acquire();

	auto config_key = get_config_key(offer_hash);

	if (sdk::has_key(config_key))
	{
		abort();
	}

	sdk::set_raw_memory(config_key, c);

	auto balance_key = get_balance_key(offer_hash);

	sdk::int64_set_add(balance_key, c.sellAmount, 0);

	erc20::Ierc20 sell_token(c.sellToken);
	sell_token.transferFrom(c.owner, sdk::get_self(), c.sellAmount);
}

void consume_offer(sdk::Hash const& offer_hash, int64_t amount_of_sell_consumed)
{
	if (amount_of_sell_consumed < 0) {
		abort();
	}

	auto config_key = get_config_key(offer_hash);
	auto balance_key = get_balance_key(offer_hash);

	auto c = sdk::get_raw_memory<offer_config>(config_key);

	int64_t required_payment = get_buy_amount(amount_of_sell_consumed, c.price);

	erc20::Ierc20 sell_token(c.sellToken);
	erc20::Ierc20 buy_token(c.buyToken);

	sell_token.transferFrom(sdk::get_self(), sdk::get_msg_sender(), amount_of_sell_consumed);
	buy_token.transferFrom(sdk::get_msg_sender(), c.owner, required_payment);

	sdk::int64_add(balance_key, -amount_of_sell_consumed);

	if (sdk::int64_get(balance_key) == 0)
	{
		sdk::delete_last(balance_key);
		sdk::delete_last(config_key);
	}
}

void cancel_offer(sdk::Hash const& offer_hash)
{
	auto config_key = get_config_key(offer_hash);
	auto balance_key = get_balance_key(offer_hash);

	int64_t remaining_bal = sdk::int64_get(balance_key);

	sdk::int64_add(balance_key, -remaining_bal);

	sdk::delete_last(balance_key);

	auto c = sdk::get_raw_memory<offer_config>(config_key);
	sdk::delete_last(config_key);

	if (c.owner != sdk::get_msg_sender())
	{
		abort();
	}

	erc20::Ierc20 sell_token(c.sellToken);
	sell_token.transferFrom(sdk::get_self(), c.owner, remaining_bal);
}


// owner is implicitly required to be sdk::msg_sender() here
EXPORT("pub00000000")
create_offer()
{
	auto calldata = sdk::get_calldata<offer_config>();
	create_offer(calldata);
}

struct calldata_consume {
	sdk::Hash hash;
	int64_t amount_sell_consumed;
};

EXPORT("pub01000000")
consume_offer()
{
	auto calldata = sdk::get_calldata<calldata_consume>();
	consume_offer(calldata.hash, calldata.amount_sell_consumed);
}

EXPORT("pub02000000")
cancel_offer()
{
	auto calldata = sdk::get_calldata<sdk::Hash>();
	cancel_offer(calldata);
}

}
