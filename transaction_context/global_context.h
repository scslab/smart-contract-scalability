#pragma once

#include "contract_db/contract_db.h"

#include "tx_block/tx_block.h"

namespace scs
{

class GlobalContext
{
	ContractDB contract_db;
	TxBlock tx_block;

public:

	GlobalContext();

	ContractDB& get_contract_db()
	{
		return contract_db;
	}

	TxBlock& get_tx_block()
	{
		return tx_block;
	}
};

class StaticGlobalContext
{
	inline static GlobalContext* context;

	friend void _free_global_context();

public:
	// takes ownership of c
	void set(GlobalContext* c);

	GlobalContext& get()
	{
		return *context;
	}
};

void
__attribute__((destructor))
_free_global_context();

} /* scs */
