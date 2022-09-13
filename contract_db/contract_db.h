#pragma once

#include "xdr/types.h"

#include <map>
#include <memory>
#include <atomic>
#include <cstdint>

#include <wasm_api/wasm_api.h>

#include "contract_db/uncommitted_contracts.h"

namespace scs {

class TxSet;

using xdr::operator==;

class ContractDB : public wasm_api::ScriptDB {

	static_assert(sizeof(wasm_api::Hash) == sizeof(Address), "mismatch between addresses in scs and addresses in wasm api");

	using contract_map_t = std::map<wasm_api::Hash, std::unique_ptr<const Contract>>;

	contract_map_t contracts;

	ContractDB(ContractDB&) = delete;
	ContractDB& operator=(ContractDB&) = delete;
	ContractDB(ContractDB&&) = delete;

	UncommittedContracts uncommitted_contracts;

public:

	ContractDB() 
		: contracts()
		{}

	const std::vector<uint8_t>*
	get_script(wasm_api::Hash const& addr, const wasm_api::script_context_t& context) const override final;

	bool
	register_contract(Address const& addr, std::unique_ptr<const Contract>&& contract);

	UncommittedContracts& get_uncommitted_proxy()
	{
		return uncommitted_contracts;
	}

	void commit(const TxSet& tx_set)
	{
		uncommitted_contracts.add_valid_contracts(contracts, tx_set);
	}
};


} /* scs */
