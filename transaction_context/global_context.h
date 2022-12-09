#pragma once

#include "contract_db/contract_db.h"

#include "rpc/rpc_address_db.h"

#include "state_db/state_db.h"
#include "state_db/modified_keys_list.h"

#include "tx_block/tx_set.h"

#include <utils/non_movable.h>

namespace scs
{

struct GlobalContext : public utils::NonMovableOrCopyable
{
	ContractDB contract_db;
	StateDB state_db;
	RpcAddressDB address_db;

	GlobalContext()
		: contract_db()
		, state_db()
		, address_db()
		{}
};

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
};

} /* scs */
