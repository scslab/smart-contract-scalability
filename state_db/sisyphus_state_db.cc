#include "state_db/sisyphus_state_db.h"
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

namespace scs
{

void 
SisyphusStateDB::SisyphusStateMetadata::from_value(value_t const& obj)
{
	auto const& res = obj.get_committed_object();
	if (res)
	{
		if (res-> body.type() == ObjectType::KNOWN_SUPPLY_ASSET)
		{
			asset_supply = res->body.asset().amount;
		} else
		{
			asset_supply = 0;
		}
	} else {
		asset_supply = 0;
	}
	trie::SnapshotTrieMetadataBase::from_value(obj);
}

SisyphusStateDB::SisyphusStateMetadata& 
SisyphusStateDB::SisyphusStateMetadata::operator+=(const SisyphusStateMetadata& other)
{
	asset_supply += other.asset_supply;
	trie::SnapshotTrieMetadataBase::operator+=(other);
	return *this;
}

std::optional<StorageObject>
SisyphusStateDB::get_committed_value(const AddressAndKey& a) const
{
    auto const* res = state_db.get_value(a);
    if (res) {
        return (res)->get_committed_object();
    }
    return std::nullopt;
}

void
do_nothing_if_merge(
    const SisyphusStateDB::value_t& value) {}

std::optional<RevertableObject::DeltaRewind>
SisyphusStateDB::try_apply_delta(const AddressAndKey& a, const StorageDelta& delta)
{
    auto* res = state_db.get_value(a);

    if (!res) {
        auto* root = state_db.get_root_and_invalidate_hash();

        root->template insert<&do_nothing_if_merge>(
                a, state_db.get_gc());

        res = state_db.get_value(a);
    }

    if (res == nullptr) {
        throw std::runtime_error("should be impossible");
    }

    return (res)->try_add_delta(delta);
}

void 
SisyphusStateDB::commit_modifications(const TypedModificationIndex& list)
{
	throw std::runtime_error("unimpl");
}

void
SisyphusStateDB::rewind_modifications(const TypedModificationIndex& list)
{
	throw std::runtime_error("unimplemented");
}

Hash
SisyphusStateDB::hash()
{
	auto h = state_db.hash_and_normalize();
    Hash out;
    std::memcpy(out.data(), h.data(), h.size());
    return out;
}

}
