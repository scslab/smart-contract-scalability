#pragma once

#include "vm/vm.h"

#include "xdr/types.h"

#include <cstdint>
#include <random>
#include <mutex>

namespace scs
{

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

	SignedTransaction
	make_deploy_wallet_transaction(size_t idx, Hash const& wallet_contract_hash, Address const& token_addr, uint16_t size_inc);

	std::pair<std::vector<SignedTransaction>, std::vector<SignedTransaction>>
	make_accounts_and_mints();

	SignedTransaction make_random_payment(uint64_t expiration_time, uint64_t nonce, std::minstd_rand& gen);

public:

	PaymentExperiment(size_t num_accounts, uint16_t hs_size_inc = 0);

	std::unique_ptr<VirtualMachine>
	prepare_vm();

	std::vector<SignedTransaction>
	gen_transaction_batch(size_t batch_size, uint64_t expiration_time = UINT64_MAX);
};

}
