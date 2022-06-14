#include "contract_db/uncommitted_contracts.h"

namespace scs
{

void
UncommittedContracts::add_valid_contracts(std::map<wasm_api::Hash, std::unique_ptr<const Contract>>& existing_contracts, const TxBlock& tx_block)
{
	for (auto& [addr, mentry] : contracts)
	{
		throw std::runtime_error("UncommittedContracts::add_valid_contracts unimpl when nonempty");
	}
}

} /* scs */
