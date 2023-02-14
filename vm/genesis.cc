/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vm/genesis.h"

#include "contract_db/contract_db.h"
#include "contract_db/contract_db_proxy.h"

#include "storage_proxy/transaction_rewind.h"

#include "utils/load_wasm.h"

#include "cpp_contracts/sdk/shared.h"

namespace scs {

Address
make_address(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
    auto buf = make_static_32bytes<std::array<uint8_t, 32>>(a, b, c, d);
    Address out;
    std::memcpy(out.data(), buf.data(), out.size());
    return out;
}

void
install_contract(ContractDBProxy& db,
                 std::shared_ptr<const Contract> contract,
                 const Address& addr)
{
    auto h = db.create_contract(contract);

    if (!db.deploy_contract(addr, h)) {
        throw std::runtime_error("failed to deploy genesis contract");
    }
}

void
install_genesis_contracts(ContractDB& contract_db)
{
    ContractDBProxy proxy(contract_db);

    // address registry should mirror cpp_contracts/genesis/addresses.h
    install_contract(proxy,
                     load_wasm_from_file("cpp_contracts/genesis/deploy.wasm"),
                     DEPLOYER_ADDRESS);

    {
        TransactionRewind rewind;

        if (!proxy.push_updates_to_db(rewind)) {
            throw std::runtime_error("failed to push proxy updates");
        }

        rewind.commit();
    }

    contract_db.commit();
}

} // namespace scs
