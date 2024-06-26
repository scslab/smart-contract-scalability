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

#include "experiments/payment_experiment.h"

#include "utils/load_wasm.h"

#include "vm/genesis.h"

#include "contract_db/contract_utils.h"
#include "utils/make_calldata.h"

#include "crypto/crypto_utils.h"
#include "crypto/hash.h"

namespace scs {


const uint64_t gas_limit = 10'000'000;

// wasmsig needs A LOT of gas
const char* payment_contract = "cpp_contracts/payment_experiment/payment_wasmsig.wasm";

PaymentExperiment::PaymentExperiment(size_t num_accounts, uint16_t hs_size_inc)
    : num_accounts(num_accounts)
    , hs_size_inc(hs_size_inc)
{}

TxSetEntry make_txset_entry(SignedTransaction const& stx) {
    TxSetEntry out;
    out.tx = stx;
    out.nondeterministic_results.push_back(NondeterministicResults());
    return out;
}

Block
make_create_transactions(const char* erc20_contract)
{
    auto erc20 = load_wasm_from_file(erc20_contract);
    auto wallet
        = load_wasm_from_file(payment_contract);

    auto make_tx = [](std::shared_ptr<const Contract> contract) -> TxSetEntry {
        struct calldata_create
        {
            uint32_t idx;
        };

        calldata_create data{ .idx = 0 };

        TransactionInvocation invocation(
            DEPLOYER_ADDRESS, 2, make_calldata(data));

        SignedTransaction stx;
        stx.tx.invocation = invocation;
        stx.tx.gas_limit = 5'000'000 + gas_limit;

        stx.tx.contracts_to_deploy.push_back(*contract);

        return make_txset_entry(stx);
    };

    Block out;
    out.transactions = { make_tx(erc20), make_tx(wallet) };
    return out;
}

TxSetEntry
make_deploy_erc20_transaction(const char* erc20_contract)
{
    auto c = load_wasm_from_file(erc20_contract);

    Hash h = hash_xdr(*c);

    struct calldata_Deploy
    {
        Hash contract_hash;
        uint64_t nonce;
    };

    calldata_Deploy data{ .contract_hash = h, .nonce = UINT64_MAX };

    TransactionInvocation invocation(DEPLOYER_ADDRESS, 1, make_calldata(data));

    SignedTransaction stx;
    stx.tx.invocation = invocation;
    stx.tx.gas_limit = gas_limit;

    return make_txset_entry(stx);
}

TxSetEntry
PaymentExperiment::make_deploy_wallet_transaction(
    size_t idx,
    Hash const& wallet_contract_hash,
    Address const& token_addr,
    uint16_t size_inc)
{
    auto [sk, pk] = deterministic_key_gen(idx);

    Address addr = compute_contract_deploy_address(
        DEPLOYER_ADDRESS, wallet_contract_hash, idx);

    {
        std::lock_guard lock(mtx);

        account_map[idx]
            = account_entry{ .pk = pk, .sk = sk, .wallet_address = addr };
    }

    struct calldata_init
    {
        Address token;
        PublicKey pk;
        uint16_t size_increase;
    };

    struct calldata_DeployAndInitialize
    {
        Hash contract_hash;
        uint64_t nonce;
        uint32_t ctor_method;
        uint32_t ctor_calldata_size;
    };

    calldata_DeployAndInitialize data{ .contract_hash = wallet_contract_hash,
                                       .nonce = idx,
                                       .ctor_method = 0,
                                       .ctor_calldata_size
                                       = sizeof(calldata_init) };

    calldata_init ctor_data{ .token = token_addr,
                             .pk = pk,
                             .size_increase = size_inc };

    auto calldata = make_calldata(data, ctor_data);

    TransactionInvocation invocation(DEPLOYER_ADDRESS, 0, calldata);

    SignedTransaction stx;
    stx.tx.invocation = invocation;
    stx.tx.gas_limit = gas_limit;

    return make_txset_entry(stx);
}

TxSetEntry
make_mint_transaction(Address const& wallet_addr,
                      Address const& token_addr,
                      int64_t amount = UINT32_MAX)
{
    struct calldata_mint
    {
        Address recipient;
        int64_t mint;
    };

    calldata_mint data{ .recipient = wallet_addr, .mint = amount };

    auto calldata = make_calldata(data);

    TransactionInvocation invocation(token_addr, 2, calldata);

    SignedTransaction stx;
    stx.tx.invocation = invocation;
    stx.tx.gas_limit = gas_limit;

    return make_txset_entry(stx);
}

std::pair<std::vector<TxSetEntry>, std::vector<TxSetEntry>>
PaymentExperiment::make_accounts_and_mints(const char* erc20_contract)
{
    std::vector<TxSetEntry> accounts_out;
    std::vector<TxSetEntry> mints_out;

    Hash wallet_contract_hash = hash_xdr(
        *load_wasm_from_file(payment_contract));
    Hash token_contract_hash
        = hash_xdr(*load_wasm_from_file(erc20_contract));

    Address token_addr = compute_contract_deploy_address(
        DEPLOYER_ADDRESS, token_contract_hash, UINT64_MAX);

    accounts_out.resize(num_accounts);
    mints_out.resize(num_accounts);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_accounts), [&](auto r) {
        for (auto i = r.begin(); i < r.end(); i++) {
            Address wallet_addr = compute_contract_deploy_address(
                DEPLOYER_ADDRESS, wallet_contract_hash, i);
            accounts_out[i] = make_deploy_wallet_transaction(
                i, wallet_contract_hash, token_addr, hs_size_inc);
            mints_out[i] = make_mint_transaction(wallet_addr, token_addr);
        }
    });

    return { accounts_out, mints_out };
}

std::vector<AddressAndKey> 
PaymentExperiment::get_active_key_set()
{
    std::vector<AddressAndKey> keys;
    keys.resize(num_accounts * 5);
    Hash wallet_contract_hash = hash_xdr(
        *load_wasm_from_file(payment_contract));

    InvariantKey pkaddr = make_static_key(1);
    InvariantKey replayaddr = make_static_key(0);
    InvariantKey token_addr = make_static_key(0, 2);

    const char* erc20_contract = (SisyphusStateDB::USE_ASSETS == 1) ? "cpp_contracts/sisyphus_erc20.wasm" : "cpp_contracts/erc20.wasm"; 

    Hash token_contract_hash
        = hash_xdr(*load_wasm_from_file(erc20_contract));

    Address token_deploy_addr = compute_contract_deploy_address(
        DEPLOYER_ADDRESS, token_contract_hash, UINT64_MAX);

    auto make_balance_key = [&] (Hash const& account) -> InvariantKey {
        xdr::opaque_vec<MAX_HASH_LEN> key;
        key.insert(key.end(),
            account.begin(),
            account.end());
        return hash_xdr(key);
    };

    auto make_allowance_key = [&] (Hash const& owner, Hash const& auth) -> InvariantKey 
    {
        xdr::opaque_vec<MAX_HASH_LEN> key;
        key.insert(key.end(),
            owner.begin(),
            owner.end());
        key.insert(key.end(),
            auth.begin(),
            auth.end());
        return hash_xdr(key);
    };


    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_accounts),
     [&](auto r) {
        for (auto i = r.begin(); i < r.end(); i++) {
            auto account = compute_contract_deploy_address(
                    DEPLOYER_ADDRESS, wallet_contract_hash, i);

            AddressAndKey addrkey;
            std::memcpy(addrkey.data(), account.data(), account.size());
            std::memcpy(addrkey.data() + account.size(), pkaddr.data(), pkaddr.size());
            keys[5*i] = addrkey;
            std::memcpy(addrkey.data() + account.size(), replayaddr.data(), replayaddr.size());
            keys[5*i + 1] = addrkey;
            std::memcpy(addrkey.data() + account.size(), token_addr.data(), token_addr.size());
            keys[5*i + 2] = addrkey;
            std::memcpy(addrkey.data(), token_deploy_addr.data(), token_deploy_addr.size());
            auto bk = make_balance_key(account);
            std::memcpy(addrkey.data() + token_deploy_addr.size(),bk.data(), bk.size());
            keys[5*i + 3] = addrkey;
            auto ak = make_allowance_key(account, account);
            std::memcpy(addrkey.data() + token_deploy_addr.size(), ak.data(), ak.size());
            keys[5*i + 4] = addrkey;
        }
    });

    return keys;
}


SignedTransaction
PaymentExperiment::make_random_payment(uint64_t expiration_time,
                                       uint64_t nonce,
                                       std::minstd_rand& gen)
{
    auto gen_account = [&]() {
        std::uniform_int_distribution<> account_dist(0, account_map.size() - 1);

        uint64_t out = account_dist(gen);

        return out;
    };
    // bizarre case of gen_account() always returning 0 on first invocation
    gen_account();

    uint64_t src_acct = gen_account();
    uint64_t dst_acct = gen_account();
    while (src_acct == dst_acct) {
        dst_acct = gen_account();
    }

    auto const& src = account_map.at(src_acct);
    auto const& dst = account_map.at(dst_acct);

    struct calldata_transfer
    {
        Address to;
        int64_t amount;
        uint64_t nonce;
        uint64_t expiration;
    };

    calldata_transfer calldata{ .to = dst.wallet_address,
                                .amount = 100,
                                .nonce = nonce,
                                .expiration = expiration_time };

    TransactionInvocation invocation(
        src.wallet_address, 1, make_calldata(calldata));

    SignedTransaction stx;
    stx.tx.invocation = invocation;
    stx.tx.gas_limit = gas_limit;

    Hash msg = hash_xdr(stx.tx);
    Signature sig = sign_ed25519(src.sk, msg);

    WitnessEntry wit;
    wit.key = 0;
    wit.value.insert(wit.value.end(), sig.begin(), sig.end());

    stx.witnesses.push_back(wit);

    return stx;
}

std::unique_ptr<VirtualMachine>
PaymentExperiment::prepare_vm()
{
    auto vm = std::make_unique<VirtualMachine>();

    vm->init_default_genesis();

    if (!vm->try_exec_tx_block(make_create_transactions("cpp_contracts/erc20.wasm"))) {
        return nullptr;
    }

    std::printf("made create txs\n");

    Block deploy_erc20;
    deploy_erc20.transactions
        = { make_deploy_erc20_transaction("cpp_contracts/erc20.wasm") };

    if (!vm->try_exec_tx_block(deploy_erc20)) {
        return nullptr;
    }

    std::printf("made deploy erc20\n");

    auto [wallet_txs, mint_txs] = make_accounts_and_mints("cpp_contracts/erc20.wasm");

    const size_t batch_exec_size = 100'000;
    for (size_t i = 0; i < (wallet_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "wallet exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), wallet_txs.size());

        block.transactions.insert(
            block.transactions.end(), wallet_txs.begin() + start, wallet_txs.begin() + end);
        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("made wallets\n");

    for (size_t i = 0; i < (mint_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "mint exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), mint_txs.size());

        block.transactions.insert(block.transactions.end(), mint_txs.begin() + start, mint_txs.begin() + end);

        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("funded wallets\n");

    return vm;
}

std::unique_ptr<GroundhogVirtualMachine>
PaymentExperiment::prepare_groundhog_vm()
{
    auto vm = std::make_unique<GroundhogVirtualMachine>();

    vm->init_default_genesis();

    if (!vm->try_exec_tx_block(make_create_transactions("cpp_contracts/erc20.wasm"))) {
        return nullptr;
    }

    std::printf("made create txs\n");

    Block deploy_erc20;
    deploy_erc20.transactions
        = { make_deploy_erc20_transaction("cpp_contracts/erc20.wasm") };

    if (!vm->try_exec_tx_block(deploy_erc20)) {
        return nullptr;
    }

    std::printf("made deploy erc20\n");

    auto [wallet_txs, mint_txs] = make_accounts_and_mints("cpp_contracts/erc20.wasm");

    const size_t batch_exec_size = 100'000;
    for (size_t i = 0; i < (wallet_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "wallet exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), wallet_txs.size());

        block.transactions.insert(
            block.transactions.end(), wallet_txs.begin() + start, wallet_txs.begin() + end);
        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("made wallets\n");

    for (size_t i = 0; i < (mint_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "mint exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), mint_txs.size());

        block.transactions.insert(block.transactions.end(), mint_txs.begin() + start, mint_txs.begin() + end);

        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("funded wallets\n");

    return vm;
}

std::unique_ptr<SisyphusVirtualMachine>
PaymentExperiment::prepare_sisyphus_vm()
{
    auto vm = std::make_unique<SisyphusVirtualMachine>();

    const char* erc20_contract = (SisyphusStateDB::USE_ASSETS == 1) ? "cpp_contracts/sisyphus_erc20.wasm" : "cpp_contracts/erc20.wasm"; //"cpp_contracts/sisyphus_erc20.wasm"; // version with asset implementation

    vm->init_default_genesis();

    std::printf("preparing sisyphus vm with erc20 contact= %s\n", erc20_contract);

    if (!vm->try_exec_tx_block(make_create_transactions(erc20_contract))) {
        return nullptr;
    }

    std::printf("made create txs\n");

    Block deploy_erc20;
    deploy_erc20.transactions
        = { make_deploy_erc20_transaction(erc20_contract) };

    if (!vm->try_exec_tx_block(deploy_erc20)) {
        return nullptr;
    }

    std::printf("made deploy erc20\n");

    auto [wallet_txs, mint_txs] = make_accounts_and_mints(erc20_contract);

    const size_t batch_exec_size = 100'000;
    for (size_t i = 0; i < (wallet_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "wallet exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), wallet_txs.size());

        block.transactions.insert(
            block.transactions.end(), wallet_txs.begin() + start, wallet_txs.begin() + end);
        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("made wallets\n");

    for (size_t i = 0; i < (mint_txs.size() / batch_exec_size) + 1; i++) {
        std::fprintf(stderr, "mint exec batch %lu\n", i);
        Block block;
        size_t start = batch_exec_size * i;
        size_t end = std::min(batch_exec_size * (i + 1), mint_txs.size());

        block.transactions.insert(block.transactions.end(), mint_txs.begin() + start, mint_txs.begin() + end);

        if (!vm->try_exec_tx_block(block)) {
            return nullptr;
        }
    }

    std::printf("funded wallets\n");

    return vm;
}


std::vector<SignedTransaction>
PaymentExperiment::gen_transaction_batch(size_t batch_size,
                                         uint64_t expiration_time)
{
    std::vector<SignedTransaction> out;
    out.resize(batch_size);

    tbb::parallel_for(tbb::blocked_range<size_t>(0, out.size()), [&](auto r) {
        std::minstd_rand gen(r.begin());
        for (auto i = r.begin(); i < r.end(); i++) {
            out[i] = make_random_payment(expiration_time, i, gen);
        }
    });

    return out;
}

} // namespace scs
