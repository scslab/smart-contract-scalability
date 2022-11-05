#pragma once

#include "vm/vm.h"

#include "xdr/types.h"

#include <cstdint>

namespace scs
{

class PaymentExperiment
{
	size_t num_accounts;

	struct account_entry
	{
		PublicKey pk;
		SecretKey sk;
		Address wallet_address;
	};

	std::map<uint64_t, account_entry> account_map;

	SignedTransaction
	make_deploy_wallet_transaction(size_t idx, Hash const& wallet_contract_hash, Address const& token_addr);

	std::vector<SignedTransaction>
	make_accounts();

public:

	PaymentExperiment(size_t num_accounts);

	std::unique_ptr<VirtualMachine>
	prepare_vm();

	std::vector<SignedTransaction>
	gen_transaction_batch(size_t batch_size);
};

}