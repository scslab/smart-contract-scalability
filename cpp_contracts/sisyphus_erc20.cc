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
#include "sdk/asset.h"

#include "sisyphus_erc20.h"

namespace sisyphus_erc20
{

using sdk::Address;
using sdk::StorageKey;

namespace internal {

static std::array<uint8_t, 64> allowance_key_buf;

static StorageKey storage_key_buf;

constexpr static std::array<uint8_t, 32> owner_id_storage_key
    = sdk::make_static_key(0, 1);
constexpr static std::array<uint8_t, 32> metadata_key
    = sdk::make_static_key(1, 1);


metadata
get_metadata()
{
    return sdk::get_raw_memory<metadata>(metadata_key);
}

void
set_metadata(const metadata& m)
{
    sdk::set_raw_memory(metadata_key, m);
}

void owner_guard()
{
    if (sdk::get_msg_sender() != sdk::get_raw_memory<sdk::Address>(owner_id_storage_key))
    {
        abort();
    }
}

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
    sdk::asset_add(storage_key_buf, -amount);

    calculate_balance_key(to, storage_key_buf);
    sdk::asset_add(storage_key_buf, amount);
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
    sdk::asset_add(storage_key_buf, amount);
}

int64_t
balance_of(const Address& addr)
{
    calculate_balance_key(addr, storage_key_buf);

    return sdk::asset_get(storage_key_buf);
}

} // namespace internal

EXPORT("pub00000000")
initialize()
{
    auto calldata = sdk::get_calldata<calldata_ctor>();

    sdk::set_raw_memory(internal::owner_id_storage_key, calldata.owner);
}

EXPORT("pub01000000")
transferFrom()
{
    auto calldata = sdk::get_calldata<calldata_transferFrom>();
    Address sender = sdk::get_msg_sender();

    internal::allowance_delta(calldata.from, sender, -calldata.amount);
    internal::transfer(calldata.from, calldata.to, calldata.amount);
}

EXPORT("pub02000000")
mint()
{
    auto calldata = sdk::get_calldata<calldata_mint>();
    internal::mint(calldata.recipient, calldata.amount);
}

EXPORT("pub03000000")
allowanceDelta()
{
    auto calldata = sdk::get_calldata<calldata_allowanceDelta>();
    Address sender = sdk::get_msg_sender();

    internal::allowance_delta(sender, calldata.account, calldata.amount);
}

EXPORT("pub04000000")
balanceOf()
{
    auto calldata = sdk::get_calldata<calldata_balanceOf>();
    sdk::return_value(internal::balance_of(calldata.account));
}

EXPORT("pub05000000")
get_metadata()
{
    auto calldata = sdk::get_calldata<calldata_get_metadata>();
    sdk::return_value(internal::get_metadata());
}

EXPORT("pub06000000")
set_metadata()
{
    auto calldata = sdk::get_calldata<metadata>();

    internal::owner_guard();
    internal::set_metadata(calldata);
}

}
