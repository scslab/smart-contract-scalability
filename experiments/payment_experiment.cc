#include "experiments/payment_experiment.h"

#include "utils/load_wasm.h"

#include "vm/genesis.h"

#include "utils/make_calldata.h"
#include "contract_db/contract_utils.h"

#include "crypto/crypto_utils.h"
#include "crypto/hash.h"

namespace scs
{

PaymentExperiment::PaymentExperiment(size_t num_accounts)
	: num_accounts(num_accounts)
	{}

std::vector<SignedTransaction>
make_create_transactions()
{
	auto erc20 = load_wasm_from_file("cpp_contracts/erc20.wasm");
	auto wallet = load_wasm_from_file("cpp_contracts/payment_experiment/payment.wasm");

	auto make_tx = [] (std::shared_ptr<const Contract> contract)
	{
		struct calldata_create {
			uint32_t idx;
		};

		calldata_create data {
			.idx = 0
		};

		TransactionInvocation invocation(
			DEPLOYER_ADDRESS,
			2,
			make_calldata(data));

		SignedTransaction stx;
		stx.tx.invocation = invocation;

		stx.tx.contracts_to_deploy.push_back(*contract);

		return stx;
	};

	return {make_tx(erc20), make_tx(wallet)};
}

SignedTransaction
make_deploy_erc20_transaction()
{
	auto c = load_wasm_from_file("cpp_contracts/erc20.wasm");

	Hash h = hash_xdr(*c);

	struct calldata_Deploy
	{
		Hash contract_hash;
		uint64_t nonce;
	};

	calldata_Deploy data
    {
        .contract_hash = h,
        .nonce = UINT64_MAX
    };

    TransactionInvocation invocation(DEPLOYER_ADDRESS, 1, make_calldata(data));

    SignedTransaction stx;
    stx.tx.invocation = invocation;

    return stx;
}

SignedTransaction
PaymentExperiment::make_deploy_wallet_transaction(size_t idx, Hash const& wallet_contract_hash, Address const& token_addr)
{
	auto [sk, pk] = deterministic_key_gen(idx);

	Address addr = compute_contract_deploy_address(DEPLOYER_ADDRESS, wallet_contract_hash, idx);

	account_map[idx] = account_entry
	{
		.pk = pk,
		.sk = sk,
		.wallet_address = addr
	};

	struct calldata_init
	{
	    Address token;
	   	PublicKey pk;
	};


	struct calldata_DeployAndInitialize
	{
		Hash contract_hash;
		uint64_t nonce;
		uint32_t ctor_method;
		uint32_t ctor_calldata_size;
	};

	calldata_DeployAndInitialize data
	{
		.contract_hash = wallet_contract_hash,
		.nonce = idx,
		.ctor_method = 0,
		.ctor_calldata_size = sizeof(calldata_init)
	};

	calldata_init ctor_data
	{
		.token = token_addr,
		.pk = pk
	};

	auto calldata = make_calldata(data, ctor_data);

	TransactionInvocation invocation(DEPLOYER_ADDRESS, 0, calldata);

    SignedTransaction stx;
    stx.tx.invocation = invocation;

    return stx;
}

std::vector<SignedTransaction>
PaymentExperiment::make_accounts()
{
	std::vector<SignedTransaction> out;

	Hash wallet_contract_hash = hash_xdr(*load_wasm_from_file("cpp_contracts/payment_experiment/payment.wasm"));
	Hash token_contract_hash = hash_xdr(*load_wasm_from_file("cpp_contracts/erc20.wasm"));

	Address token_addr = compute_contract_deploy_address(DEPLOYER_ADDRESS, token_contract_hash, UINT64_MAX);

	for (size_t i = 0; i < num_accounts; i++)
	{
		out.push_back(make_deploy_wallet_transaction(i, wallet_contract_hash, token_addr));
	}

	return out;
}

std::unique_ptr<VirtualMachine>
PaymentExperiment::prepare_vm()
{
	auto vm = std::make_unique<VirtualMachine>();

	vm -> init_default_genesis();

	if (!vm -> try_exec_tx_block(make_create_transactions()))
	{
		return nullptr;
	}

	std::printf("make create txs\n");

	std::vector<SignedTransaction> deploy_erc20 = { make_deploy_erc20_transaction() };

	if (!vm -> try_exec_tx_block(deploy_erc20))
	{
		return nullptr;
	}

	std::printf("made deploy erc20\n");

	auto wallet_txs = make_accounts();

	if (!vm -> try_exec_tx_block(wallet_txs))
	{
		return nullptr;
	}

	return vm;
}

}