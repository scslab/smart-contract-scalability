#pragma once

#include "xdr/storage.h"
#include "xdr/storage_delta.h"
#include "xdr/types.h"

#include <cstdint>
#include <memory>
#include <vector>

#include <wasm_api/wasm_api.h>

namespace scs
{

class TxBlock;

class UncommittedContracts
{
	struct map_entry_t
	{
		DeltaPriority p;
		Hash src_tx;
		std::unique_ptr<const Contract> contract;
	};

	std::map<wasm_api::Hash, map_entry_t> contracts;

public:

	bool
	register_contract(Address const& addr, std::unique_ptr<const Contract>&& contract, const DeltaPriority& p, const Hash& src_tx, TxBlock& tx_block)
	{
		throw std::runtime_error("UncommittedContracts::register_contract unimplemented");
	}

	const std::vector<uint8_t>*
	get_script(wasm_api::Hash const& addr, const Hash& src_tx) const
	{
		return nullptr;
	}

	void
	add_valid_contracts(std::map<wasm_api::Hash, std::unique_ptr<const Contract>>& existing_contracts, const TxBlock& tx_block)
	{
		for (auto& [addr, mentry] : contracts)
		{
			throw std::runtime_error("UncommittedContracts::add_valid_contracts unimpl when nonempty");
		}
	}
};

} /* scs */
