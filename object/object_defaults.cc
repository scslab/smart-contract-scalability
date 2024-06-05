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

#include "object/object_defaults.h"

#include "crypto/hash.h"

#include <cstring>

namespace scs
{

StorageObject
object_from_delta_class(StorageDeltaClass const& dc, std::optional<StorageObject> const& prev_object)
{
	StorageObject out;
	out.body.type(dc.type());

	if (prev_object)
	{
		if (prev_object -> body.type() != out.body.type())
		{
			throw std::runtime_error("type mismatch in object_from_delta_class");
		}
	}
	switch(out.body.type())
	{
		case ObjectType::RAW_MEMORY:
		{
			out.body.raw_memory_storage().data = dc.data();
			break;
		}
		case ObjectType::NONNEGATIVE_INT64:
		{
			out.body.nonnegative_int64() = dc.nonnegative_int64();
			break;
		}
		case ObjectType::HASH_SET:
		{
			out.body.hash_set().max_size = START_HASH_SET_SIZE;

			if (prev_object)
			{
				out.body.hash_set().max_size = prev_object -> body.hash_set().max_size;
				out.body.hash_set().hashes = prev_object -> body.hash_set().hashes;
			}
			break;
		}
		case ObjectType::KNOWN_SUPPLY_ASSET:
		{
			out.body.asset().amount = 0;
			if (prev_object)
			{
				out.body.asset().amount = prev_object -> body.asset().amount;
			}
			break;
		}
		break;
		default:
			throw std::runtime_error("unimpl sdc object_defaults.cc");
	}
	return out;
}

StorageDeltaClass 
delta_class_from_delta(StorageDelta const& delta)
{
	StorageDeltaClass out;

	switch(delta.type())
	{
	case DeltaType::RAW_MEMORY_WRITE:
		out.type(ObjectType::RAW_MEMORY);
		out.data() = delta.data();
		return out;
	case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		out.type(ObjectType::NONNEGATIVE_INT64);
		out.nonnegative_int64() = delta.set_add_nonnegative_int64().set_value;
		return out;
	case DeltaType::HASH_SET_INSERT:
	case DeltaType::HASH_SET_INCREASE_LIMIT:
	case DeltaType::HASH_SET_CLEAR:
	case DeltaType::HASH_SET_INSERT_RESERVE_SIZE:
		out.type(ObjectType::HASH_SET);
		return out;
	case DeltaType::ASSET_OBJECT_ADD:
		out.type(ObjectType::KNOWN_SUPPLY_ASSET);
		return out;
	case DeltaType::DELETE_LAST:
		throw std::runtime_error("Delete Last has no typeclass");
	}

	throw std::runtime_error("invalid delta type in create StorageDeltaClass");
}

CompressedStorageDeltaClass
compressed_delta_class_from_delta(StorageDelta const& delta)
{
	CompressedStorageDeltaClass out;

	switch(delta.type())
	{
	case DeltaType::RAW_MEMORY_WRITE:
		out.type(ObjectType::RAW_MEMORY);
		// key difference
		hash_raw(delta.data().data(), delta.data().size(), out.hashed_data().data());
		return out;
	case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		out.type(ObjectType::NONNEGATIVE_INT64);
		out.nonnegative_int64() = delta.set_add_nonnegative_int64().set_value;
		return out;
	case DeltaType::HASH_SET_INSERT:
	case DeltaType::HASH_SET_INCREASE_LIMIT:
	case DeltaType::HASH_SET_CLEAR:
	case DeltaType::HASH_SET_INSERT_RESERVE_SIZE:
		out.type(ObjectType::HASH_SET);
		return out;
	case DeltaType::ASSET_OBJECT_ADD:
		out.type(ObjectType::KNOWN_SUPPLY_ASSET);
		return out;
	case DeltaType::DELETE_LAST:
		throw std::runtime_error("DELETE_LAST has no typeclass");
	}

	throw std::runtime_error("invalid delta type in create StorageDeltaClass");
}

} /* scs */
