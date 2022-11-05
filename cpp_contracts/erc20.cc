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

namespace erc20
{

using sdk::Address;
using sdk::StorageKey;

namespace internal {

static std::array<uint8_t, 64> allowance_key_buf;

static StorageKey storage_key_buf;

constexpr static std::array<uint8_t, 32> owner_id_storage_key
    = sdk::make_static_key(0, 1);

constexpr static std::array<uint8_t, 32> total_supply_storage_key
    = sdk::make_static_key(1, 1);

void
calculate_balance_key(const Address& addr, StorageKey& o_key)
{
    sdk::hash(addr, o_key);
}

void
calculate_allowance_key(const Address& owner,
                        const Address& auth,
                        StorageKey& o_key)
{
    static_assert(sizeof(owner) + sizeof(auth) == allowance_key_buf.size());

    std::memcpy(allowance_key_buf.data(), owner.data(), owner.size());
    std::memcpy(
        allowance_key_buf.data() + owner.size(), auth.data(), auth.size());

    sdk::hash(allowance_key_buf, o_key);
}

void
transfer(const Address& from, const Address& to, const int64_t amount)
{
    if (amount < 0) {
        abort();
    }

    calculate_balance_key(from, storage_key_buf);
    sdk::int64_add(storage_key_buf, -amount);

    calculate_balance_key(to, storage_key_buf);
    sdk::int64_add(storage_key_buf, amount);
}

void
allowance_delta(const Address& owner, const Address& auth, const int64_t amount)
{
    calculate_allowance_key(owner, auth, storage_key_buf);
    sdk::int64_add(storage_key_buf, amount);
}

void
mint(const Address& to, const int64_t amount)
{
    if (amount < 0) {
        abort();
    }
    calculate_balance_key(to, storage_key_buf);
    sdk::int64_add(storage_key_buf, amount);

    sdk::int64_add(total_supply_storage_key, amount);
}

} // namespace internal

EXPORT("pub00000000")
transferFrom()
{
    auto calldata = sdk::get_calldata<calldata_transferFrom>();
    Address sender = sdk::get_msg_sender();

    internal::allowance_delta(calldata.from, sender, -calldata.amount);
    internal::transfer(calldata.from, calldata.to, calldata.amount);
}

EXPORT("pub01000000")
mint()
{
    auto calldata = sdk::get_calldata<calldata_mint>();
    internal::mint(calldata.recipient, calldata.amount);
}

EXPORT("pub02000000")
allowanceDelta()
{
    auto calldata = sdk::get_calldata<calldata_allowanceDelta>();
    Address sender = sdk::get_msg_sender();

    internal::allowance_delta(sender, calldata.account, calldata.amount);
}

}
