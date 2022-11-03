#pragma once

#include "transaction_context/global_context.h"

#include <memory>
#include <vector>

#include "xdr/transaction.h"

namespace scs
{

class VirtualMachine
{

	GlobalContext global_context;
	std::unique_ptr<BlockContext> current_block_context;

	bool
	validate_tx_block(std::vector<Transaction> const& txs);

	void advance_block_number();

public:

	bool
	try_exec_tx_block(std::vector<Transaction> const& txs);

};

}