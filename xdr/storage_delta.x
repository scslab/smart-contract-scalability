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

%#include "xdr/types.h"
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE_LAST = 0,
	RAW_MEMORY_WRITE = 0x10,
	NONNEGATIVE_INT64_SET_ADD = 0x20,
	HASH_SET_INSERT = 0x30,
	HASH_SET_INCREASE_LIMIT = 0x31,
	HASH_SET_CLEAR = 0x32,
	HASH_SET_INSERT_RESERVE_SIZE = 0x33,
	ASSET_OBJECT_ADD = 0x40
};

% static_assert(static_cast<uint64_t>(-1) == 0xFFFF'FFFF'FFFF'FFFF, "requires two's complement");

struct set_add_t
{
       int64 set_value;
       int64 delta;
};

union StorageDelta switch (DeltaType type)
{
	case DELETE_LAST:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64_SET_ADD:
		set_add_t set_add_nonnegative_int64;
	case HASH_SET_INSERT:
		HashSetEntry hash;
	case HASH_SET_INCREASE_LIMIT:
		// enforced to be at most uint16_max
		// (MAX_HASH_SET_SIZE)
		uint32 limit_increase;
	case HASH_SET_CLEAR:
		uint64 threshold;
	case HASH_SET_INSERT_RESERVE_SIZE:
		uint32 reserve_amount;
	case ASSET_OBJECT_ADD:
		int64 asset_delta;
};

struct PrioritizedStorageDelta {
	StorageDelta delta;
	uint64_t priority;
};

// REFUND DESIGN:
// imo this is the simplest
// alternative is tracking excesses per block
// into "refundable events" and then paying these back
// afterwards, but that seems like a lot of extra work.
// 
// on each StorageObject, track "invested/locked units",
// then give a "claim_all_extra" action that users can do
// (once per block, per addrkey) to reclaim any extra

// All StorageDeltas on one object must all
// modify the same ObjectType
// and must agree on the information contained within 
// a StorageDeltaClass
// TODO: A more careful equivalence relation
// on this class would allow things like hash set limit set,
// not just add().

union StorageDeltaClass switch (ObjectType type)
{
	case RAW_MEMORY:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64:
		int64 nonnegative_int64;
	case HASH_SET:
		void;
	case KNOWN_SUPPLY_ASSET:
		void;
};

union CompressedStorageDeltaClass switch (ObjectType type)
{
	case RAW_MEMORY:
		Hash hashed_data;  // only difference
	case NONNEGATIVE_INT64:
		int64 nonnegative_int64;
	case HASH_SET:
		void;
	case KNOWN_SUPPLY_ASSET:
		void;
};


struct IndexedModification
{
    AddressAndKey addr;
    Hash tx_hash;
    StorageDelta delta;
};

typedef IndexedModification ModIndexLog<>;


} /* scs */
