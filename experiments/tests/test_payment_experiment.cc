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

}