#pragma once

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

#include "contract_db/contract_db.h"

#include "rpc/rpc_address_db.h"

#include "state_db/state_db.h"
#include "state_db/state_db_v2.h"
#include "state_db/groundhog_persistent_state_db.h"
#include "state_db/sisyphus_state_db.h"
#include "state_db/modified_keys_list.h"
#include "state_db/typed_modification_index.h"

#include "tx_block/unique_tx_set.h"
#include "tx_block/tx_set.h"

#include <utils/non_movable.h>
#include <wasm_api/wasm_api.h>

namespace scs
{

struct GlobalContext : public utils::NonMovableOrCopyable
{
	ContractDB contract_db;
	StateDB state_db;
	RpcAddressDB address_db;
	wasm_api::SupportedWasmEngine engine;

	GlobalContext(wasm_api::SupportedWasmEngine engine_inp)
		: engine(engine_inp) {}
};

struct SisyphusGlobalContext : public utils::NonMovableOrCopyable
{
	ContractDB contract_db;
	SisyphusStateDB state_db;
	RpcAddressDB address_db;
	wasm_api::SupportedWasmEngine engine;

	SisyphusGlobalContext(wasm_api::SupportedWasmEngine engine_inp)
		: engine(engine_inp) {}
};

struct GroundhogGlobalContext : public utils::NonMovableOrCopyable
{
	ContractDB contract_db;
	GroundhogPersistentStateDB state_db;
	RpcAddressDB address_db;
	wasm_api::SupportedWasmEngine engine;

	GroundhogGlobalContext(wasm_api::SupportedWasmEngine engine_inp)
		: engine(engine_inp) {}
};

template<typename T>
class TransactionContext;

typedef TransactionContext<GlobalContext> TxContext;
typedef TransactionContext<GroundhogGlobalContext> GroundhogTxContext;
typedef TransactionContext<SisyphusGlobalContext> SisyphusTxContext;

struct BlockContext : public utils::NonMovableOrCopyable
{
	TxSet tx_set;
	ModifiedKeysList modified_keys_list;
	uint64_t block_number;

	BlockContext(uint64_t block_number)
		: tx_set()
		, modified_keys_list()
		, block_number(block_number)
		{}

	void reset_context(uint64_t new_block_number)
	{
		block_number = new_block_number;
		tx_set.clear();
		modified_keys_list.clear();
	}
	using tx_context_t = TxContext;
};

struct GroundhogBlockContext : public utils::NonMovableOrCopyable
{
	TxSet tx_set;
	ModifiedKeysList modified_keys_list;
	uint64_t block_number;

	GroundhogBlockContext(uint64_t block_number)
		: tx_set()
		, modified_keys_list()
		, block_number(block_number)
		{}

	void reset_context(uint64_t new_block_number)
	{
		block_number = new_block_number;
		tx_set.clear();
		modified_keys_list.clear();
	}
	using tx_context_t = GroundhogTxContext;
};

struct SisyphusBlockContext : public utils::NonMovableOrCopyable
{
	UniqueTxSet tx_set;
	TypedModificationIndex modified_keys_list;
	uint64_t block_number;

	SisyphusBlockContext(uint64_t block_number)
		: tx_set()
		, modified_keys_list()
		, block_number(block_number)
		{}

	void reset_context(uint64_t new_block_number)
	{
		block_number = new_block_number;
		tx_set.clear();
		modified_keys_list.clear();
	}
	using tx_context_t = SisyphusTxContext;
};

} /* scs */
