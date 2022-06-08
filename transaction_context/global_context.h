#pragma once

#include "contract_db/contract_db.h"

#include "tx_block/tx_block.h"
#include "state_db/delta_batch.h"
#include "state_db/state_db.h"

namespace scs
{

struct BlockContext
{
	TxBlock tx_block;
	DeltaBatch delta_batch;
};

struct GlobalContext
{
	ContractDB contract_db;
	StateDB state_db;

	BlockContext current_block_context;

	GlobalContext()
		: contract_db()
		, state_db()
		, current_block_context()
		{}

	GlobalContext(const GlobalContext&) = delete;
	GlobalContext(GlobalContext&&) = delete;
	GlobalContext& operator=(const GlobalContext&) = delete;
};

} /* scs */
