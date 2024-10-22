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

#include <gtest/gtest.h>

#include "experiments/payment_experiment.h"

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

#include <tbb/global_control.h>

namespace scs
{

TEST(PaymentExperiment, InitializeSmall)
{
	PaymentExperiment e(2);

	auto vm = e.prepare_vm();

	ASSERT_TRUE(!!vm);
}

TEST(PaymentExperiment, SmallBlockOk)
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

	ASSERT_TRUE(!!vm);


	auto batch = e.gen_transaction_batch(10);

	Block b;
	for (auto const& stx : batch)
	{
		b.transactions.push_back(make_txset(stx));
	}

	ASSERT_TRUE(vm -> try_exec_tx_block(b));
}

TEST(PaymentExperiment, LargeBlockFillsReplayCache)
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

	ASSERT_TRUE(!!vm);


	auto batch = e.gen_transaction_batch(200);

	Block b;
	for (auto const& stx : batch)
	{
		b.transactions.push_back(make_txset(stx));
	}

	ASSERT_FALSE(vm -> try_exec_tx_block(b));
}


TEST(PaymentExperiment, AssembleBlockSmall)
{
	PaymentExperiment e(10, 1000);

	auto vm = e.prepare_vm();

	ASSERT_TRUE(!!vm);

	auto& mp = vm -> get_mempool();
	ASSERT_TRUE(mp.add_txs(e.gen_transaction_batch(10000)) == 10000);

	AssemblyLimits limits(10, INT64_MAX);

	Block blk;

	auto header = vm -> propose_tx_block(limits, 1000, 10, blk);

	ASSERT_EQ(blk.transactions.size(), 10);
}

TEST(PaymentExperiment, AssembleSeveralBlocks)
{
	PaymentExperiment e(10, 1000);

	auto vm = e.prepare_vm();

	ASSERT_TRUE(!!vm);

	auto& mp = vm -> get_mempool();
	ASSERT_EQ(mp.add_txs(e.gen_transaction_batch(10000)), 10000);

	Block blk;

	for (size_t i = 0; i < 10; i++)
	{
		AssemblyLimits limits(100, INT64_MAX);
		std::printf("============= start block ===============\n");
		auto header = vm -> propose_tx_block(limits, 1000, 10, blk);
		ASSERT_EQ(blk.transactions.size(),  100);
	}
} 


} // namespace scs
