#pragma once

#include "contract_db/contract_db.h"

#include "tx_block/tx_block.h"
#include "state_db/delta_batch.h"
#include "state_db/state_db.h"

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

} /* scs */
