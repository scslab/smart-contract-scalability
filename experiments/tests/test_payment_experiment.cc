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

	auto make_txset = [] (SignedTransaction const& stx)
	{
		TxSetEntry out;
		out.tx = stx;
		out.nondeterministic_results.push_back(NondeterministicResults());
		return out;
	};

	REQUIRE(!!vm);

	SECTION("small block ok")
	{
		auto batch = e.gen_transaction_batch(10);

		Block b;
		for (auto const& stx : batch)
		{
			b.transactions.push_back(make_txset(stx));
		}

		REQUIRE(vm -> try_exec_tx_block(b));
	}

	SECTION("large blocks fill replay cache")
	{
		auto batch = e.gen_transaction_batch(200);

		Block b;
		for (auto const& stx : batch)
		{
			b.transactions.push_back(make_txset(stx));
		}

		REQUIRE(!vm -> try_exec_tx_block(b));
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

} // namespace scs
