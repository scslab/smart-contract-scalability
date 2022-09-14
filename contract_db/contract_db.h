#pragma once

#include "xdr/types.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>

#include <wasm_api/wasm_api.h>

#include "contract_db/uncommitted_contracts.h"

#include "mtt/utils/non_movable.h"

namespace scs {

class BlockContext;

using xdr::operator==;

class ContractDB
    : public wasm_api::ScriptDB
    , public utils::NonMovableOrCopyable
{

    static_assert(
        sizeof(wasm_api::Hash) == sizeof(Address),
        "mismatch between addresses in scs and addresses in wasm api");

    using contract_map_t
        = std::map<wasm_api::Hash, std::unique_ptr<const Contract>>;

    contract_map_t contracts;

    UncommittedContracts uncommitted_contracts;

  public:
    ContractDB();

    const std::vector<uint8_t>* get_script(
        wasm_api::Hash const& addr,
        const wasm_api::script_context_t& context) const override final;

    bool register_contract(Address const& addr,
                           std::unique_ptr<const Contract>&& contract);

    UncommittedContracts& get_uncommitted_proxy()
    {
        return uncommitted_contracts;
    }

    void commit(const BlockContext& block_context);
};

} // namespace scs
