#include <catch2/catch_test_macros.hpp>

#include "experiments/payment_experiment.h"

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

namespace scs
{

TEST_CASE("payment experiment init", "[experiment][payment]")
{
	PaymentExperiment e(2);

	auto vm = e.prepare_vm();

	REQUIRE(!!vm);
}

TEST_CASE("payment experiment hashset", "[experiment][payment]")
{
	PaymentExperiment e(2);

	auto vm = e.prepare_vm();

	REQUIRE(!!vm);

	SECTION("small block ok")
	{
		// we don't have expiration times yet in the replay prevention sdk
		auto batch = e.gen_transaction_batch(10);

		REQUIRE(vm -> try_exec_tx_block(batch));
	}

	SECTION("large blocks fill replay cache")
	{
		auto batch = e.gen_transaction_batch(200);

		REQUIRE(!vm -> try_exec_tx_block(batch));
	}
}

TEST_CASE("payment experiment assemble block", "[now][payment]")
{
	std::printf("start fn\n");
	PaymentExperiment e(10);

	auto vm = e.prepare_vm();

	REQUIRE(!!vm);

	//SECTION("prepare one small block")
	{
		auto& mp = vm -> get_mempool();
		REQUIRE(mp.add_txs(e.gen_transaction_batch(10000)) == 10000);

		AssemblyLimits limits(10, INT64_MAX);

		auto [header, blk] = vm -> propose_tx_block(limits, 1000, 10);

		REQUIRE(blk.transactions.size() == 10);
	}
}

}