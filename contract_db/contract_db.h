#pragma once

#include "xdr/types.h"

#include <map>
#include <memory>

#include <wasm_api/wasm_api.h>

namespace scs {

using xdr::operator==;

class ContractDB : public wasm_api::ScriptDB {

	static_assert(sizeof(wasm_api::Hash) == sizeof(Address), "mismatch between addresses in scs and addresses in wasm api");

	std::map<wasm_api::Hash, std::unique_ptr<const Contract>> contracts;

	ContractDB(ContractDB&) = delete;
	ContractDB& operator=(ContractDB&) = delete;
	ContractDB(ContractDB&&) = delete;

public:

	ContractDB() : contracts() {}

	const std::vector<uint8_t>*
	get_script(wasm_api::Hash const& addr) const override final;

	bool
	register_contract(Address const& addr, std::unique_ptr<const Contract>&& contract);
};

} /* scs */
