#pragma once

#include "xdr/types.h"

#include <map>
#include <memory>

namespace scs {

using xdr::operator==;

class ContractDB {
	std::map<Address, std::shared_ptr<const Contract>> contracts;

	ContractDB(ContractDB&) = delete;
	ContractDB& operator=(ContractDB&) = delete;

public:

	ContractDB() : contracts() {}

	std::shared_ptr<const Contract>
	get_contract(Address const& addr)
	{
		auto it = contracts.find(addr);
		if (it == contracts.end())
		{
			return nullptr;
		}
		return it->second;
	}
};

} /* scs */
