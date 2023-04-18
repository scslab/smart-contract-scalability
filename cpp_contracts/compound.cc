#include "sdk/alloc.h"
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/semaphore.h"
#include "sdk/general_storage.h"

#include "erc20.h"

#include "compound.h"

// units of price: 32 bits * 2^-8
// base asset price is always 1
/**
 * In the internal computations: we always multiply price (raw) by quantity, and then >> 8.
 * This loses some rounding, but is simple.
 */

// base asset balances are stored as deltas relative to (INT64_MAX-1)/2
// maintain invariant that asset supply to contract (so total supplied to one account, at max)
// is at most INT64_MAX - base offset
// other balances stored normally

namespace compound
{

using int128_t = __int128;
using uint128_t = unsigned __int128;

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
	sdk::Address administrator;
	uint64_t reserve_target;
	std::array<asset_config, MAX_ASSETS> assets;
	uint8_t active_asset_count;

	uint8_t storefront_discount; // discount / 1<<8
	uint64_t last_updated_block;

	interest_rate_params interest;
};

struct compound_indices
{
	uint64_t base_supply_index;
	uint64_t base_borrow_index;
	uint64_t last_updated_block;
};

int128_t present_value_supply(int64_t principal, const compound_indices& config)
{
	return (static_cast<int128_t>(principal) * config.base_supply_index) >> INDEX_OFFSET;
}

int128_t present_value_borrow(int64_t principal, const compound_indices& config)
{
	return (static_cast<int128_t>(principal) * config.base_borrow_index) >> INDEX_OFFSET;
}

int64_t principal_value_supply(int64_t present, const compound_indices& config)
{
	return (static_cast<int128_t>(present) << INDEX_OFFSET) / config.base_supply_index;
}

int64_t principal_value_borrow(int64_t present, const compound_indices& config)
{
	return (static_cast<int128_t>(present) << INDEX_OFFSET) / config.base_borrow_index;
}

struct user_config
{
	uint64_t last_updated_block;
};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
constexpr static sdk::StorageKey index_addr = sdk::make_static_key(1, 2);

// units have to be in principal, not present
constexpr static sdk::StorageKey base_supply_addr = sdk::make_static_key(2, 2);
constexpr static sdk::StorageKey base_borrow_addr = sdk::make_static_key(3, 2);

constexpr static sdk::StorageKey base_supply_cap_addr = sdk::make_static_key(4, 2);
constexpr static sdk::StorageKey base_borrow_cap_addr = sdk::make_static_key(5, 2);

// units in present value
constexpr static sdk::StorageKey reserve_target_addr = sdk::make_static_key(6, 2);

constexpr static sdk::StorageKey semaphore_key = sdk::make_static_key(7, 2);


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
	user_asset_addr_buf[32] = 252;
	return sdk::hash(user_asset_addr_buf);
}

sdk::StorageKey user_base_asset_borrow_cap_addr(const sdk::Address& addr)
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

sdk::StorageKey asset_for_sale_supply_addr(uint8_t asset)
{
	static_assert(2 * MAX_ASSETS <= UINT8_MAX, "OVERFLOW");
	return sdk::hash(asset + MAX_ASSETS);
}

void
adjust_supply_cap(uint8_t asset, int64_t amount)
{
	// initializes to 0
	sdk::int64_add(asset_supply_cap_addr(asset), amount);
}

uint64_t get_utilization(const compound_indices& indices)
{
	int128_t borrow = present_value_borrow(sdk::int64_get(base_borrow_addr), indices);
	int128_t supply = present_value_supply(sdk::int64_get(base_supply_addr), indices);

	if (supply == 0)
	{
		return 0;
	}

	// assumption is that borrow won't overflow int96;
	return (borrow << 32) / supply;
}

// utilization measured out of 1<<32
// output: measured out of 1 << INDEX_OFFSET
uint64_t 
get_supply_rate(uint64_t utilization, const interest_rate_params& config)
{
	if (utilization < config.supplyKink)
	{
		uint128_t output = (static_cast<uint128_t>(config.supplyRateBase) << 32) + static_cast<uint128_t>(utilization) * static_cast<uint128_t>(config.supplyRateLow);
		return output >> 32;
	}
	else
	{
		uint128_t output = (static_cast<uint128_t>(config.supplyRateBase) << 32) + static_cast<uint128_t>(config.supplyKink) * static_cast<uint128_t>(config.supplyRateLow)
			+ static_cast<uint128_t>(utilization - config.supplyKink) * static_cast<uint128_t>(config.supplyRateHigh);
		return output >> 32;
	}
}

// utilization measured out of 1<<32
// output: measured out of 1 << INDEX_OFFSET
uint64_t 
get_borrow_rate(uint64_t utilization, const interest_rate_params& config)
{
	if (utilization < config.supplyKink)
	{
		uint128_t output = (static_cast<uint128_t>(config.borrowRateBase) << 32) + static_cast<uint128_t>(utilization) * static_cast<uint128_t>(config.borrowRateLow);
		return output >> 32;
	}
	else
	{
		uint128_t output = (static_cast<uint128_t>(config.borrowRateBase) << 32) + static_cast<uint128_t>(config.borrowKink) * static_cast<uint128_t>(config.borrowRateLow)
			+ static_cast<uint128_t>(utilization - config.borrowKink) * static_cast<uint128_t>(config.borrowRateHigh);
		return output >> 32;
	}
}
const compound_indices
refresh_compound_indices(const compound_config& config)
{
	auto indices = sdk::get_raw_memory<compound_indices>(index_addr);

	if (indices.last_updated_block == sdk::get_block_number())
	{
		return indices;
	}

	// cannot update prices (or edit config) and also use that
	// new config when managing state updates to other parts
	// of the contract, in the same block
	// (i.e. constraints would be set based on old prices, but
	// adjustments would be based on new prices.  Could
	// choose to set constraints based on new prices,
	// but this causes excessive conflicts between price-setting tx and everything
	// else.  Why should the price setting transaction also do
	// borrowing/lending?  No good reason for this).
	if (config.last_updated_block == sdk::get_block_number())
	{
		abort();
	}

	uint64_t elapsed_time = sdk::get_block_number() - indices.last_updated_block;

	indices.last_updated_block = sdk::get_block_number();

	// calculate interest indices
	uint32_t utilization = get_utilization(indices);

	// measured as 1/1<<INDEX_OFFSET
	uint64_t supply_rate = get_supply_rate(utilization, config.interest);

	indices.base_supply_index += 
		(static_cast<uint128_t>(indices.base_supply_index) 
		* static_cast<uint128_t> (supply_rate)
		* static_cast<uint128_t> (elapsed_time)) >> INDEX_OFFSET;

	uint64_t borrow_rate = get_borrow_rate(utilization, config.interest);

	indices.base_borrow_index += 
		(static_cast<uint128_t>(indices.base_borrow_index) 
		* static_cast<uint128_t> (borrow_rate)
		* static_cast<uint128_t> (elapsed_time)) >> INDEX_OFFSET;

	// calculate reserve target
	erc20::Ierc20 base_token(config.base_token_addr);
	int128_t balance = base_token.balanceOf(sdk::get_self());

	balance += present_value_borrow(sdk::int64_get(base_borrow_addr), indices)
		- present_value_supply(sdk::int64_get(base_supply_addr), indices);

	balance = static_cast<int128_t>(config.reserve_target) - balance;

	if (balance < INT64_MAX)
	{
		balance = INT64_MAX;
	}
	if (balance > INT64_MIN)
	{
		balance = INT64_MIN;
	}
	sdk::int64_set_add(reserve_target_addr, balance, 0); // target - (erc20 balance + borrowed - supplied)

	sdk::set_raw_memory(index_addr, indices);
	return indices;
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

int64_t compute_user_borrow_constraint_base(const sdk::Address& user, const compound_config& config, const compound_indices& indices)
{

	sdk::StorageKey asset = user_base_asset_borrow_addr(user);
	int128_t base = -present_value_borrow(sdk::int64_get(asset), indices);

	asset = user_base_asset_supply_addr(user);
	base += present_value_supply(sdk::int64_get(asset), indices);

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

void set_base_user_borrow_constraint(const sdk::Address& user, const compound_config& config, const compound_indices& indices)
{
	auto key = user_config_addr(user);

	auto u_conf = sdk::get_raw_memory<user_config>(key);

	if (u_conf.last_updated_block == sdk::get_block_number())
	{
		return;
	}

	u_conf.last_updated_block = sdk::get_block_number();

	sdk::set_raw_memory(key, u_conf);

	int64_t base_constraint = compute_user_borrow_constraint_base(user, config, indices);

	sdk::int64_set_add(user_borrow_constraint_addr(user), base_constraint, 0);
}

void adjust_user_borrow_constraint(const sdk::Address& user, uint8_t asset, int64_t amount, const compound_config& config)
{
	int64_t delta = compute_user_borrow_constraint_delta(asset, amount, config);

	sdk::int64_add(user_borrow_constraint_addr(user), delta);
}

void adjust_user_borrow_base_constraint(const sdk::Address& user, int64_t amount)
{
	int64_t delta = compute_user_borrow_base_constraint_delta(amount);
	sdk::int64_add(user_borrow_constraint_addr(user), delta);
}

void user_adjust_borrow(const sdk::Address& addr, int64_t present_value_amount, const compound_indices& indices)
{
	int64_t principal_value = principal_value_borrow(present_value_amount, indices);
	sdk::int64_add(user_base_asset_borrow_addr(addr), principal_value);

	auto cap_key = user_base_asset_borrow_cap_addr(addr);

	if (!sdk::has_key(cap_key))
	{
		sdk::int64_set(cap_key, INT64_MAX);
	}
	sdk::int64_add(cap_key, -principal_value);

	adjust_user_borrow_base_constraint(addr, -present_value_amount);

	sdk::int64_add(base_borrow_addr, principal_value);
	sdk::int64_add(base_borrow_cap_addr, -principal_value);
}

void user_adjust_supply(const sdk::Address& addr, int64_t present_value_amount, const compound_indices& indices, bool adjust_borrow_constraint = true)
{
	int64_t principal_value = principal_value_supply(present_value_amount, indices);
	sdk::int64_add(user_base_asset_supply_addr(addr), principal_value);

	if (adjust_borrow_constraint)
	{
		adjust_user_borrow_base_constraint(addr, present_value_amount);
	}

	sdk::int64_add(base_supply_addr, principal_value);
	sdk::int64_add(base_supply_cap_addr, -principal_value);
}


bool is_liquidatable(const sdk::Address& user, const compound_config& config, const compound_indices& indices)
{
	sdk::StorageKey asset = user_base_asset_borrow_addr(user);
	int128_t base = -present_value_borrow(sdk::int64_get(asset), indices);

	asset = user_base_asset_supply_addr(user);
	base += present_value_supply(sdk::int64_get(asset), indices);

	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		asset = user_asset_addr(user, i);
		int128_t balance = (sdk::int64_get(asset)) * config.assets[i].liquidity_frac;
		base += (balance * static_cast<int128_t>(config.assets[i].price)) >> 8; // for the borrowing frac
	}

	return (base < 0);
}

void absorb_account(const sdk::Address& account, const compound_config& config, const compound_indices& indices)
{
	// this method reduces a user's borrow constraint, so we need to special case the borrow constraint modifications.
	if (!is_liquidatable(account, config, indices))
	{
		abort();
	}

	// at this point, the constraint is strictly negative
	set_base_user_borrow_constraint(account, config, indices);

	sdk::StorageKey asset;

	int64_t balance_add = 0;

	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		asset = user_asset_addr(account, i);
		int64_t seized = sdk::int64_get(asset);
		sdk::int64_add(asset, -seized); // set to 0, without accidental race condition

		// seize asset, log as available for sale
		asset = asset_for_sale_supply_addr(i);
		sdk::int64_add(asset, seized);

		int64_t payment = ((static_cast<int128_t>(seized) * config.assets[i].price) * config.assets[i].liquidity_frac) >> (8+8);
		balance_add += payment;

		sdk::int64_add(asset_supply_cap_addr(i), -seized);
	}

	user_adjust_supply(account, balance_add, indices, false);
	//sdk::int64_add(user_base_asset_supply_addr(account), principal_value_supply(balance_add, indices));
	//sdk::int64_add(base_supply_addr, principal_value_supply(balance_add, indices));

	sdk::int64_add(reserve_target_addr, -balance_add);
}

uint64_t
quote_collateral(uint64_t base_amount, uint8_t asset, const compound_config& config)
{
	// original:
	//  uint256 discountFactor = mulFactor(storeFrontPriceFactor, FACTOR_SCALE - assetInfo.liquidationFactor);
    //   uint256 assetPriceDiscounted = mulFactor(assetPrice, FACTOR_SCALE - discountFactor);
    // in other words:
    // price * (1-(storefront * (1-liquidation)) = price * (1 - storefront + liquidation*storefront)

	// leftshifted by 16
	uint64_t discount = 
		(static_cast<uint64_t>(1) << 16) 
		- (config.storefront_discount * ((static_cast<uint64_t>(1)<<8) - config.assets[asset].liquidity_frac));

	uint128_t sell_price = (static_cast<uint128_t>(config.assets[asset].price) * static_cast<uint128_t>(discount)) >> 16;

	return ((static_cast<uint128_t>(base_amount) << 64) / sell_price) >> (64-16);
}

// need indices ref not directly for data,
// but to ensure reserve_target constraint is updated.
void buy_collateral(
	const sdk::Address& account,
	uint8_t asset, uint64_t base_amount, uint64_t min_recv_asset_amount, 
	const compound_config& config,
	[[maybe_unused]] const compound_indices&)
{
	uint64_t recv_amt = quote_collateral(base_amount, asset, config);

	if (recv_amt < min_recv_asset_amount)
	{
		abort();
	}

	// sell asset
	sdk::int64_add(asset_for_sale_supply_addr(asset), -recv_amt);

	erc20::Ierc20 sold_asset(config.assets[asset].addr);
	sold_asset.transferFrom(sdk::get_self(), account, recv_amt);

	erc20::Ierc20 base_asset(config.base_token_addr);
	base_asset.transferFrom(account, sdk::get_self(), base_amount);

	sdk::int64_add(reserve_target_addr, base_amount);
}

void transfer_asset(
	const sdk::Address& from, 
	const sdk::Address& to, 
	uint8_t asset, int64_t amount, const compound_config& config, const compound_indices& indices)
{
	if (amount < 0)
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config, indices);
	set_base_user_borrow_constraint(to, config, indices);

	adjust_user_borrow_constraint(from, asset, -amount, config);
	adjust_user_borrow_constraint(to, asset, amount, config);

	sdk::int64_add(user_asset_addr(from, asset), -amount);
	sdk::int64_add(user_asset_addr(to, asset), amount);
}

void pos_quantity_add_overflow(int64_t a, int64_t b)
{
	if (a < 0 || b < 0)
	{
		abort();
	}
	if (INT64_MAX - a - b < 0)
	{
		abort();
	}
}

void transfer_base(
	const sdk::Address& from, const sdk::Address& to, int64_t amount_from_borrow, int64_t amount_from_supply, 
	const compound_config& config,
	const compound_indices& indices)
{
	// amount_from_borrow INCREASES borrowed amount
	// amount_from_supply DECREASES supplied amount

	pos_quantity_add_overflow(amount_from_borrow, amount_from_supply);

	set_base_user_borrow_constraint(from, config, indices);
	set_base_user_borrow_constraint(to, config, indices);

	user_adjust_borrow(from, amount_from_borrow, indices);
	user_adjust_supply(from, -amount_from_supply, indices);

	user_adjust_supply(to, amount_from_borrow + amount_from_supply, indices);
}

void normalize_account(const sdk::Address& address, int64_t amount, const compound_indices& indices)
{
	user_adjust_borrow(address, -amount, indices);
	user_adjust_supply(address, -amount, indices);
}

void withdraw_base(const sdk::Address& from, const sdk::Address& to, int64_t borrow_amount, 
	int64_t supply_amount, const compound_config& config, const compound_indices& indices)
{
	pos_quantity_add_overflow(borrow_amount, supply_amount);

	// borrow amount INCREASES borrow
	// supply amount DECREASES supply

	set_base_user_borrow_constraint(from, config, indices);

	user_adjust_borrow(from, borrow_amount, indices);
	user_adjust_supply(from, -supply_amount, indices);

	erc20::Ierc20 base_token(config.base_token_addr);

	base_token.transferFrom(sdk::get_self(), to, borrow_amount + supply_amount);
}

void withdraw_asset(
	const sdk::Address& from, const sdk::Address& to, uint8_t asset, int64_t amount, 
	const compound_config& config,
	const compound_indices& indices)
{
	if (amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(from, config, indices);
	adjust_user_borrow_constraint(from, asset, -amount, config);

	auto asset_key = user_asset_addr(from, asset);

	sdk::int64_add(asset_key, -amount);
	sdk::int64_add(asset_supply_cap_addr(asset), amount);

	erc20::Ierc20 asset_token(config.assets[asset].addr);

	asset_token.transferFrom(sdk::get_self(), to, amount);
}

void supply_base(
	const sdk::Address& from, const sdk::Address& to, int64_t borrow_amount, int64_t supply_amount, 
	const compound_config& config,
	const compound_indices& indices)
{
	pos_quantity_add_overflow(borrow_amount, supply_amount);

	set_base_user_borrow_constraint(to, config, indices);

	user_adjust_borrow(from, -borrow_amount, indices);
	user_adjust_supply(from, supply_amount, indices);

	erc20::Ierc20 base_token(config.base_token_addr);

	base_token.transferFrom(from, sdk::get_self(), borrow_amount + supply_amount);
}

void supply_asset(
	const sdk::Address& from, const sdk::Address& to, uint8_t asset, int64_t amount, const compound_config& config,
	const compound_indices& indices)
{
	if (amount < 0) 
	{
		abort();
	}

	set_base_user_borrow_constraint(to, config, indices);
	adjust_user_borrow_constraint(to, asset, amount, config);

	auto asset_key = user_asset_addr(to, asset);

	sdk::int64_add(asset_key, amount);

	sdk::int64_add(asset_supply_cap_addr(asset), -amount);

	erc20::Ierc20 asset_token(config.assets[asset].addr);
	asset_token.transferFrom(from, sdk::get_self(), amount);
}

void require_administrator(const compound_config& config)
{
	if (sdk::get_msg_sender() != config.administrator)
	{
		abort();
	}
}

EXPORT("pub00000000")
initialize()
{
	sdk::Semaphore s(semaphore_key);
	s.acquire();

	if (sdk::has_key(config_addr))
	{
		abort();
	}

	auto calldata = sdk::get_calldata<calldata_ctor>();

	compound_config config;
	config.base_token_addr = calldata.base_token_addr;
	config.administrator = calldata.administrator;
	config.storefront_discount = 255;

	sdk::int64_set(base_supply_cap_addr, INT64_MAX);
	sdk::int64_set(base_borrow_cap_addr, INT64_MAX);

	write_config(config);
}

EXPORT("pub01000000")
admin()
{
	auto calldata = sdk::get_calldata<calldata_admin>();

	auto config = get_config();

	require_administrator(config);

	config.reserve_target = calldata.reserve_target;
	config.storefront_discount = calldata.storefront_discount;

	config.interest = calldata.interest;

	write_config(config);
}

EXPORT("pub02000000")
list_asset()
{
	auto calldata = sdk::get_calldata<calldata_list_asset>();

	auto config = get_config();

	require_administrator(config);

	if (config.active_asset_count == MAX_ASSETS)
	{
		abort();
	}
	auto& asset = config.assets[config.active_asset_count];
	config.active_asset_count++;

	asset.addr = calldata.addr;
	asset.borrowing_frac = 0;
	asset.liquidity_frac = 0;
	asset.price = 0;

	write_config(config);
}

EXPORT("pub03000000")
update_asset_params()
{
	auto calldata = sdk::get_calldata<calldata_update_asset_params>();

	auto config = get_config();

	require_administrator(config);

	if (config.active_asset_count >= calldata.asset)
	{
		abort();
	}

	config.assets[calldata.asset].borrowing_frac = calldata.borrow_frac;
	config.assets[calldata.asset].liquidity_frac = calldata.liquidity_frac;

	write_config(config);
}

EXPORT("pub04000000")
update_prices()
{
	auto calldata = sdk::get_calldata<calldata_update_prices>();

	auto config = get_config();

	require_administrator(config);

	for (uint8_t i = 0; i < MAX_ASSETS; i++)
	{
		config.assets[i].price = calldata.new_prices[i];
	}

	write_config(config);
}

EXPORT("pub05000000")
adjust_supply_cap()
{
	auto calldata = sdk::get_calldata<calldata_adjust_supply_cap>();

	require_administrator(get_config());

	adjust_supply_cap(calldata.asset, calldata.amount);
}

EXPORT("pub06000000")
transfer_asset()
{
	auto calldata = sdk::get_calldata<calldata_transfer_asset>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	transfer_asset(sdk::get_msg_sender(), calldata.to, calldata.asset, calldata.amount, config, indices);
}

EXPORT("pub07000000")
transfer_base()
{
	auto calldata = sdk::get_calldata<calldata_transfer_base>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	transfer_base(sdk::get_msg_sender(), calldata.to, calldata.amount_from_borrow, calldata.amount_from_supply, config, indices);
}

EXPORT("pub08000000")
supply_asset()
{
	auto calldata = sdk::get_calldata<calldata_supply_asset>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	supply_asset(sdk::get_msg_sender(), calldata.to, calldata.asset, calldata.amount, config, indices);
}

EXPORT("pub09000000")
supply_base()
{
	auto calldata = sdk::get_calldata<calldata_supply_base>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	supply_base(sdk::get_msg_sender(), calldata.to, calldata.amount_from_borrow, calldata.amount_from_supply, config, indices);
}

EXPORT("pub0A000000")
withdraw_asset()
{
	auto calldata = sdk::get_calldata<calldata_withdraw_asset>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	withdraw_asset(sdk::get_msg_sender(), calldata.to, calldata.asset, calldata.amount, config, indices);
}

EXPORT("pub0B000000")
withdraw_base()
{
	auto calldata = sdk::get_calldata<calldata_withdraw_base>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	withdraw_base(sdk::get_msg_sender(), calldata.to, calldata.amount_from_borrow, calldata.amount_from_supply, config, indices);
}

EXPORT("pub0C000000")
normalize()
{
	auto calldata = sdk::get_calldata<calldata_normalize_account>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	normalize_account(sdk::get_msg_sender(), calldata.amount, indices);
}

EXPORT("pub0D000000")
absorb()
{
	auto calldata = sdk::get_calldata<calldata_absorb>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	absorb_account(calldata.account, config, indices);
}

EXPORT("pub0E000000")
buy_collateral()
{
	auto calldata = sdk::get_calldata<calldata_buy_collateral>();

	const auto config = get_config();
	const auto indices = refresh_compound_indices(config);

	buy_collateral(sdk::get_msg_sender(), calldata.asset, calldata.base_amount, calldata.min_recv_amt, config, indices);
}

} // namespace compound
