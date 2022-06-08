#include "contract_db/contract_db.h"

#include "xdr/types.h"

namespace scs
{

const std::vector<uint8_t>*
ContractDB::get_script(wasm_api::Hash const& addr, const wasm_api::script_context_t& context) const
{

	auto const* hash = static_cast<const Hash*>(context);
	auto uncommitted_res = uncommitted_contracts.get_script(addr, *hash);
	if (uncommitted_res != nullptr)
	{
		return uncommitted_res;
	}
	auto it = contracts.find(addr);
	if (it == contracts.end())
	{
		return nullptr;
	}
	return it->second.get();
}

bool 
ContractDB::register_contract(Address const& addr, std::unique_ptr<const Contract>&& contract)
{
	
	auto it = contracts.find(addr);
	if (it != contracts.end())
	{
		return false;
	}

	contracts.emplace(addr, std::move(contract));
	return true;
}


} /* scs */
