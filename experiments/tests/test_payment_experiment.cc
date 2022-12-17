#include <catch2/catch_test_macros.hpp>

#include "experiments/payment_experiment.h"

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

#include <tbb/global_control.h>

namespace scs
{

TEST_CASE("payment experiment init", "[experiment][paymentz]")
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
		auto batch = e.gen_transaction_batch(10);

		REQUIRE(vm -> try_exec_tx_block(batch));
	}

	SECTION("large blocks fill replay cache")
	{
		auto batch = e.gen_transaction_batch(200);

		REQUIRE(!vm -> try_exec_tx_block(batch));
	}
}

TEST_CASE("payment experiment assemble block", "[experiment][payments]")
{

	//tbb::global_control g(tbb::global_control::max_allowed_parallelism, 1);
	PaymentExperiment e(10, 1000);

	auto vm = e.prepare_vm();

	REQUIRE(!!vm);

	SECTION("prepare one small block")
	{
		auto& mp = vm -> get_mempool();
		REQUIRE(mp.add_txs(e.gen_transaction_batch(10000)) == 10000);

		AssemblyLimits limits(10, INT64_MAX);

		Block blk;

		auto header = vm -> propose_tx_block(limits, 1000, 10, blk);

		REQUIRE(blk.transactions.size() == 10);
	}

	SECTION("prepare several small blocks")
	{
		auto& mp = vm -> get_mempool();
		REQUIRE(mp.add_txs(e.gen_transaction_batch(10000)) == 10000);

		Block blk;

		for (size_t i = 0; i < 10; i++)
		{
			AssemblyLimits limits(100, INT64_MAX);
			std::printf("============= start block ===============\n");
			auto header = vm -> propose_tx_block(limits, 1000, 10, blk);
			REQUIRE(blk.transactions.size() == 100);
		}
	} 
}

}
