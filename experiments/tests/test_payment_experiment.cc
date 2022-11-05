#include <catch2/catch_test_macros.hpp>

#include "experiments/payment_experiment.h"

#include "vm/vm.h"

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
	std::printf("start test\n");
	PaymentExperiment e(2);

	auto vm = e.prepare_vm();

	REQUIRE(!!vm);

	std::printf("start sections\n");

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

}