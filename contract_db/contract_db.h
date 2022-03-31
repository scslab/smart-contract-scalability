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
	get_contract(Address const& addr) const;

	bool
	register_contract(Address const& addr, std::shared_ptr<const Contract> contract);
};

} /* scs */
