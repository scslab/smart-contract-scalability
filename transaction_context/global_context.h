#pragma once

#include "contract_db/contract_db.h"

#include "state_db/state_db.h"
#include "state_db/modified_keys_list.h"

#include "tx_block/tx_set.h"

#include <utils/non_movable.h>

namespace scs
{

struct GlobalContext
{
	ContractDB contract_db;
	StateDB state_db;

	GlobalContext()
		: contract_db()
		, state_db()
		{}

	GlobalContext(const GlobalContext&) = delete;
	GlobalContext(GlobalContext&&) = delete;
	GlobalContext& operator=(const GlobalContext&) = delete;
};

struct BlockContext : public utils::NonCopyable
{
	TxSet tx_set;
	ModifiedKeysList modified_keys_list;
	uint64_t block_number;

	BlockContext(uint64_t block_number)
		: tx_set()
		, modified_keys_list()
		, block_number(block_number)
		{}
};

} /* scs */
