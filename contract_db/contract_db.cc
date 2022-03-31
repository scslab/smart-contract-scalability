#include "contract_db/contract_db.h"

namespace scs
{

std::shared_ptr<const Contract>
ContractDB::get_contract(Address const& addr) const
{
	auto it = contracts.find(addr);
	if (it == contracts.end())
	{
		return nullptr;
	}
	return it->second;
}

bool 
ContractDB::register_contract(Address const& addr, std::shared_ptr<const Contract> contract)
{
	auto it = contracts.find(addr);
	if (it != contracts.end())
	{
		return false;
	}

	contracts.emplace(addr, contract);
	return true;
}



} /* scs */
