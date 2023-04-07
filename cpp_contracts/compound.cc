#include "sdk/alloc.h"
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"

#include "erc20.h"

// units of price: 32 bits * 2^-8
// base asset price is always 1 << 8

// base asset balances are stored as deltas relative to (INT64_MAX-1)/2
// maintain invariant that asset supply to contract (so total supplied to one account, at max)
// is at most INT64_MAX - base offset
// other balances stored normally

namespace internal
{

using int128_t = __int128;

constexpr static uint8_t MAX_ASSETS = 15;

struct asset_config 
{
	sdk::Address addr;
	uint32_t price;
	uint8_t borrowing_frac; // out of 2^8
	uint8_t liquidity_frac;
};

constexpr static uint8_t INDEX_OFFSET = 48;
constexpr static uint64_t INITIAL_INDEX = static_cast<uint64_t>(1)<<INDEX_OFFSET;

struct compound_config
{
	sdk::Address base_token_addr;
	std::array<asset_config, MAX_ASSETS> assets;
	uint64_t last_updated_block;

	uint64_t base_supply_index;
	uint64_t base_borrow_index;
};

int128_t present_value_supply(int64_t principal, const compound_config& config)
{
	return (static_cast<int128_t>(principal) * config.base_supply_index) >> INDEX_OFFSET;
}

int128_t present_value_borrow(int64_t principal, const compound_config& config)
{
	return (static_cast<int128_t>(principal) * config.base_borrow_index) >> INDEX_OFFSET;
}

int64_t principal_value_supply(int64_t present, const compound_config& config)
{
	return (static_cast<int128_t>(present) << INDEX_OFFSET) / config.base_supply_index;
}

int64_t principal_value_borrow(int64_t present, const compound_config& config)
{
	return (static_cast<int128_t>(present) << INDEX_OFFSET) / config.base_borrow_index;
}

struct user_config
{

};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
constexpr static sdk::StorageKey base_supply_addr = sdk::make_static_key(1, 2);
constexpr static sdk::StorageKey base_borrow_addr = sdk::make_static_key(2, 2);

sdk::StorageKey user_config_addr(const sdk::Address& addr)
{
	return sdk::hash(addr);
}

static std::array<uint8_t, 33> user_asset_addr_buf;
sdk::StorageKey user_asset_addr(const sdk::Address& addr, const uint8_t asset)
{
	std::memcpy(user_asset_addr_buf.data(), addr.data(), 32);
	user_asset_addr_buf[32] = asset;
	return sdk::hash(user_asset_addr_buf);
}
sdk::StorageKey user_base_asset_borrow_addr(const sdk::Address& addr)
{
	std::memcpy(user_asset_addr_buf.data(), addr.data(), 32);
	user_asset_addr_buf[32] = 253;
	return sdk::hash(user_asset_addr_buf);
}

sdk::StorageKey user_base_asset_supply_addr(const sdk::Address& addr)
{
	std::memcpy(user_asset_addr_buf.data(), addr.data(), 32);
	user_asset_addr_buf[32] = 254;
	return sdk::hash(user_asset_addr_buf);
}

sdk::StorageKey user_borrow_constraint_addr(const sdk::Address& addr)
{
	std::memcpy(user_asset_addr_buf.data(), addr.data(), 32);
	user_asset_addr_buf[32] = 255;
	return sdk::hash(user_asset_addr_buf);
}

sdk::StorageKey asset_supply_cap_addr(uint8_t asset)
{
	return sdk::hash(asset);
}

void
raise_supply_cap(uint8_t asset, int64_t amount)
{
	// initializes to 0
	sdk::int64_add(asset_supply_cap_addr(asset), amount);
}

void
write_config(compound_config& config)
{
	config.last_updated_block = sdk::get_block_number();
	sdk::set_raw_memory(config_addr, config);
}

compound_config get_config()
{
	return sdk::get_raw_memory<compound_config>(config_addr);
}

user_config get_user_config(const sdk::Address& user)
{
	return sdk::get_raw_memory<user_config>(user_config_addr(user));
}

void
update_prices(std::array<uint32_t, MAX_ASSETS> const& prices)
{
	auto config = get_config();
	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		config.assets[i].price = prices[i];
	}
	write_config(config);
}


int64_t compute_user_borrow_constraint_base(const sdk::Address& user, const compound_config& config)
{

	sdk::StorageKey asset = user_base_asset_borrow_addr(user);
	int128_t base = -present_value_borrow(sdk::int64_get(asset), config);

	asset = user_base_asset_supply_addr(user);
	base += present_value_supply(sdk::int64_get(asset), config);

	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		asset = user_asset_addr(user, i);
		int128_t balance = (sdk::int64_get(asset)) * config.assets[i].borrowing_frac;
		base += (balance * static_cast<int128_t>(config.assets[i].price)) >> 8; // for the borrowing frac
	}

	if (base > INT64_MAX)
	{
		return INT64_MAX;
	}
	if (base < INT64_MIN)
	{
		return INT64_MIN;
	}
	return base;
}

int64_t compute_user_borrow_constraint_delta(uint8_t asset, int64_t amount, const compound_config& config)
{
	int128_t prod = config.assets[asset].borrowing_frac * static_cast<int128_t>(amount);
	prod *= config.assets[asset].price;
	prod >>= 8;
	if (prod < INT64_MIN)
	{
		return INT64_MIN;
	}
	if (prod > INT64_MAX)
	{
		return INT64_MAX;
	}
	return prod;
}

int64_t compute_user_borrow_base_constraint_delta(int64_t amount)
{
	return amount;
}

void set_base_user_borrow_constraint(const sdk::Address& user, const compound_config& config)
{
	int64_t base_constraint = compute_user_borrow_constraint_base(user, config);

	sdk::int64_set_add(user_borrow_constraint_addr(user), base_constraint, 0);
}

void adjust_user_borrow_constraint(const sdk::Address& user, uint8_t asset, int64_t amount, const compound_config& config)
{
	int64_t delta = compute_user_borrow_constraint_delta(asset, amount, config);

	sdk::int64_add(user_borrow_constraint_addr(user), delta);
}

void adjust_user_borrow_base_constraint(const sdk::Address& user, int64_t amount, const compound_config& config)
{
	int64_t delta = compute_user_borrow_base_constraint_delta(amount);
	sdk::int64_add(user_borrow_constraint_addr(user), delta);
}

bool is_liquidatable(const sdk::Address& user, const compound_config& config)
{
	sdk::StorageKey asset = user_base_asset_borrow_addr(user);
	int128_t base = -present_value_borrow(sdk::int64_get(asset), config);

	asset = user_base_asset_supply_addr(user);
	base += present_value_supply(sdk::int64_get(asset), config);

	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		asset = user_asset_addr(user, i);
		int128_t balance = (sdk::int64_get(asset)) * config.assets[i].liquidity_frac;
		base += (balance * static_cast<int128_t>(config.assets[i].price)) >> 8; // for the borrowing frac
	}

	return (base < 0);
}

void transfer_asset(const sdk::Address& from, const sdk::Address& to, uint8_t asset, int64_t amount, const compound_config& config)
{
	if (amount < 0)
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	set_base_user_borrow_constraint(to, config);

	adjust_user_borrow_constraint(from, asset, -amount, config);
	adjust_user_borrow_constraint(to, asset, amount, config);

	sdk::int64_add(user_asset_addr(from, asset), -amount);
	sdk::int64_add(user_asset_addr(to, asset), amount);
}

void transfer_base(const sdk::Address& from, const sdk::Address& to, int64_t amount_from_borrow, int64_t amount_from_supply, const compound_config& config)
{
	if (amount_from_borrow < 0 || amount_from_supply < 0)
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	set_base_user_borrow_constraint(to, config);

	if (INT64_MAX - amount_from_supply - amount_from_borrow < 0)
	{
		abort();
	}

	adjust_user_borrow_base_constraint(from, - amount_from_borrow - amount_from_supply, config);
	adjust_user_borrow_base_constraint(to, amount_from_borrow + amount_from_supply, config);

	auto from_borrow_key = user_base_asset_borrow_addr(from);
	auto from_supply_key = user_base_asset_supply_addr(from);

	auto to_supply_key = user_base_asset_supply_addr(to);

	sdk::int64_add(from_borrow_key, principal_value_borrow(-amount_from_borrow, config));
	sdk::int64_add(from_supply_key, principal_value_borrow(-amount_from_supply, config));

	sdk::int64_add(to_supply_key, principal_value_supply(amount_from_borrow + amount_from_supply, config));
}

void normalize_account(const sdk::Address& address, int64_t amount, const compound_config& config)
{
	// does not affect borrow constraint (except maybe a rounding error), so we ignore that here
	auto borrow_key = user_base_asset_borrow_addr(address);
	auto supply_key = user_base_asset_supply_addr(address);
	
	sdk::int64_add(borrow_key, principal_value_borrow(-amount, config));
	sdk::int64_add(supply_key, principal_value_borrow(amount, config));
}

void withdraw_base(const sdk::Address& from, const sdk::Address& to, int64_t borrow_amount, int64_t supply_amount, const compound_config& config)
{
	if (borrow_amount < 0 || supply_amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	adjust_user_borrow_base_constraint(from, -borrow_amount - supply_amount, config);

	auto borrow_key = user_base_asset_borrow_addr(from);
	auto supply_key = user_base_asset_supply_addr(from);

	sdk::int64_add(borrow_key, principal_value_borrow(-borrow_amount, config));
	sdk::int64_add(supply_key, principal_value_borrow(-supply_amount, config));

	sdk::int64_add(base_borrow_addr, -borrow_amount);
	sdk::int64_add(base_supply_addr, -supply_amount);

	erc20::Ierc20 base_token(config.base_token_addr);

	base_token.transferFrom(sdk::get_self(), to, borrow_amount + supply_amount);
}

void withdraw_asset(const sdk::Address& from, const sdk::Address& to, uint8_t asset, int64_t amount, const compound_config& config)
{
	if (amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	adjust_user_borrow_constraint(from, asset, -amount, config);

	auto asset_key = user_asset_addr(from, asset);

	sdk::int64_add(asset_key, -amount);
	sdk::int64_add(asset_supply_cap_addr(asset), -amount);

	erc20::Ierc20 asset_token(config.assets[asset].addr);

	asset_token.transferFrom(sdk::get_self(), to, amount);
}

void supply_base(const sdk::Address& from, const sdk::Address& to, int64_t borrow_amount, int64_t supply_amount, const compound_config& config)
{
	if (borrow_amount < 0 || supply_amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	adjust_user_borrow_base_constraint(from, borrow_amount + supply_amount, config);

	auto borrow_key = user_base_asset_borrow_addr(from);
	auto supply_key = user_base_asset_supply_addr(from);

	sdk::int64_add(borrow_key, principal_value_borrow(borrow_amount, config));
	sdk::int64_add(supply_key, principal_value_borrow(supply_amount, config));

	sdk::int64_add(base_borrow_addr, borrow_amount);
	sdk::int64_add(base_supply_addr, supply_amount);

	erc20::Ierc20 base_token(config.base_token_addr);

	base_token.transferFrom(to, sdk::get_self(), borrow_amount + supply_amount);
}

void supply_asset(const sdk::Address& from, const sdk::Address& to, uint8_t asset, int64_t amount, const compound_config& config)
{
	if (amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config);
	adjust_user_borrow_constraint(from, asset, amount, config);

	auto asset_key = user_asset_addr(from, asset);

	sdk::int64_add(asset_key, amount);

	sdk::int64_add(asset_supply_cap_addr(asset), amount);

	erc20::Ierc20 asset_token(config.assets[asset].addr);

	asset_token.transferFrom(to, sdk::get_self(), amount);
}


}
