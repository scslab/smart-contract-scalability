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
#include "sdk/delete.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/semaphore.h"

#include "erc721.h"

namespace erc721
{

using sdk::Address;
using sdk::StorageKey;
using sdk::Hash;

namespace internal
{

static std::array<uint8_t, 64> operator_key_buf;

StorageKey
get_operator_key(Address const& owner, Address const& op)
{
    static_assert(sizeof(owner) + sizeof(op) == operator_key_buf.size());

    StorageKey out;

    std::memcpy(operator_key_buf.data(), owner.data(), owner.size());
    std::memcpy(
        operator_key_buf.data() + owner.size(), op.data(), op.size());

    sdk::hash(operator_key_buf, out);
    return out;
}

static std::array<uint8_t, 33> approval_key_buf;

StorageKey
get_approval_key(Hash const& nft)
{
	StorageKey out;
	approval_key_buf[0] = 0;
	std::memcpy(approval_key_buf.data() + 1, nft.data(), nft.size());
	sdk::hash(approval_key_buf, out);
	return out;
}

static std::array<uint8_t, 33> owner_key_buf;

StorageKey owner_key(Hash const& nft)
{
	StorageKey out;
	owner_key_buf[0] = 1;
	std::memcpy(owner_key_buf.data() + 1, nft.data(), nft.size());
	sdk::hash(owner_key_buf, out);
	return out;
}

static std::array<uint8_t, 33> transfer_semaphore_key_buf;

StorageKey get_transfer_semaphore_key(Hash const& nft)
{
	StorageKey out;
	transfer_semaphore_key_buf[0] = 2;
	std::memcpy(transfer_semaphore_key_buf.data() + 1, nft.data(), nft.size());
	sdk::hash(transfer_semaphore_key_buf, out);
	return out;
}

void
set_approval_for_all(Address const& op, bool approve)
{
	auto sender = sdk::get_msg_sender();

	StorageKey key = get_operator_key(sender, op);
	if (approve)
	{
		sdk::int64_set_add(key, 1, 0);
	} else
	{
		// alternatives:
		sdk::int64_set_add(key, 0, 0);
		//sdk::delete_last(key);
	}
}

Address get_owner(Hash const& nft)
{
	return sdk::get_raw_memory<Address>(owner_key(nft));
}

Address get_approved(Hash const& nft)
{
	return sdk::get_raw_memory<Address>(get_approval_key(nft));
}

void
approve(Hash const& nft, Address const& approved_addr)
{
	bool valid = false;

	auto owner = get_owner(nft);
	auto sender = sdk::get_msg_sender();

	if (sender == owner)
	{
		valid = true;
	} 
	else if (sdk::int64_get(get_operator_key(owner, sender)))
	{
		valid = true;
	}
	
	if (!valid)
	{
		abort();
	}

	sdk::set_raw_memory(get_approval_key(nft), approved_addr);
}

void
transferFrom(Address const& from, Address const& to, Hash const& nft)
{
	bool valid = false;
	auto sender = sdk::get_msg_sender();
	auto owner = get_owner(nft);

	if (owner != from)
	{
		abort();
	}

	if (sdk::int64_get(get_operator_key(owner, sender)))
	{
		valid = true;
	}
	else if (owner == sender)
	{
		valid = true;
	}

	if (!valid)
	{
		abort();
	}

	sdk::set_raw_memory(owner_key(nft), to);
	sdk::delete_last(get_approval_key(nft));

	sdk::TransientSemaphore(get_transfer_semaphore_key(nft)).acquire();
}

}

EXPORT("pub00000000")
approve()
{
    auto calldata = sdk::get_calldata<calldata_approve>();

    internal::approve(calldata.nft, calldata.approved);
    return 0;
}

EXPORT("pub01000000")
approveAll()
{
    auto calldata = sdk::get_calldata<calldata_approveAll>();
    internal::set_approval_for_all(calldata.op, calldata.auth);
    return 0;
}

EXPORT("pub02000000")
transferFrom()
{
    auto calldata = sdk::get_calldata<calldata_transferFrom>();
    Address sender = sdk::get_msg_sender();

    internal::transferFrom(calldata.from, calldata.to, calldata.nft);
    return 0;
}

}

