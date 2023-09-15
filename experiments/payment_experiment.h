#pragma once

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

#include "vm/vm.h"
#include "sisyphus_vm/vm.h"

#include "xdr/types.h"

#include <cstdint>
#include <mutex>
#include <random>

namespace scs {

class PaymentExperiment
{
    size_t num_accounts;
    uint16_t hs_size_inc;

    struct account_entry
    {
        PublicKey pk;
        SecretKey sk;
        Address wallet_address;
    };

    std::mutex mtx;

    std::map<uint64_t, account_entry> account_map;

    TxSetEntry make_deploy_wallet_transaction(
        size_t idx,
        Hash const& wallet_contract_hash,
        Address const& token_addr,
        uint16_t size_inc);

    std::pair<std::vector<TxSetEntry>, std::vector<TxSetEntry>>
    make_accounts_and_mints(const char* erc20_contract);

    SignedTransaction make_random_payment(uint64_t expiration_time,
                                          uint64_t nonce,
                                          std::minstd_rand& gen);

  public:
    PaymentExperiment(size_t num_accounts, uint16_t hs_size_inc = 0);

    std::unique_ptr<VirtualMachine> prepare_vm();

    std::unique_ptr<SisyphusVirtualMachine> prepare_sisyphus_vm();

    std::vector<SignedTransaction> gen_transaction_batch(
        size_t batch_size,
        uint64_t expiration_time = UINT64_MAX);
};

} // namespace scs
