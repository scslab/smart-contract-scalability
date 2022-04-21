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
};

class StaticGlobalContext
{
	inline static GlobalContext* context;

	friend void _free_global_context();

public:
	// takes ownership of c
	void set(GlobalContext* c);
};

void
__attribute__((destructor))
_free_global_context();

} /* scs */
