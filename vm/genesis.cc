#include "vm/genesis.h"

#include "contract_db/contract_db.h"
#include "contract_db/contract_db_proxy.h"

#include "storage_proxy/transaction_rewind.h"

#include "utils/load_wasm.h"

#include "cpp_contracts/sdk/shared.h"

namespace scs
{

Address make_address(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	auto buf = make_static_32bytes<std::array<uint8_t, 32>>(a, b, c, d);
	Address out;
	std::memcpy(out.data(), buf.data(), out.size());
	return out;
}

void install_contract(ContractDBProxy& db, std::shared_ptr<const Contract> contract, const Address& addr)
{

	auto h = db.create_contract(contract);

	if (!db.deploy_contract(h, addr))
	{
		throw std::runtime_error("failed to deploy genesis contract");
	}
}

void install_genesis_contracts(ContractDB& contract_db)
{
	ContractDBProxy proxy(contract_db);


	// address registry should mirror cpp_contracts/genesis/addresses.h
	install_contract(
		proxy,
		load_wasm_from_file("cpp_contracts/genesis/deploy.wasm"),
		make_address(0, 0, 0, 0));

	{
		TransactionRewind rewind;

		if (!proxy.push_updates_to_db(rewind))
		{
			throw std::runtime_error("failed to push proxy updates");
		}

		rewind.commit();
	}

	contract_db.commit();
}





}
