#pragma once

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


#include <optional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>

#include <utils/async_worker.h>

#include "transaction_context/execution_context.h"


namespace scs
{

class AssemblyLimits;
class Mempool;

template<typename GlobalContext_t, typename BlockContext_t>
class AssemblyWorker
{
	Mempool& mempool;
	GlobalContext_t& global_context;
	ExecutionContext<TransactionContext<GlobalContext_t>> exec_ctx;

	using TxContext_t = typename BlockContext_t::tx_context_t;

public:

	AssemblyWorker(Mempool& mempool, GlobalContext_t& global_context)
		: mempool(mempool)
		, global_context(global_context)
		{
		}

	void run(BlockContext_t& block_context, AssemblyLimits& limits);

	using bc_t = BlockContext_t;
};

template<typename worker_t>
class AsyncAssemblyWorker : public utils::AsyncWorker
{
	std::unique_ptr<worker_t> worker;
	typename worker_t::bc_t* current_block_context = nullptr;
	AssemblyLimits* limits = nullptr;
	bool initial_slot = false;

	bool exists_work_to_do() override final
	{
		return (current_block_context!= nullptr) && (limits != nullptr);
	}

	void run();

public:

	AsyncAssemblyWorker()
		: utils::AsyncWorker()
		, worker()
	{
		start_async_thread(
			[this] {run();}
		);
	}

	~AsyncAssemblyWorker()
	{
		terminate_worker();
	}

	template<typename... Args>
	void start_worker(typename worker_t::bc_t* cbt, AssemblyLimits* l, bool i_slot, Args& ...args)
	{
		std::lock_guard lock(mtx);
		if (!worker)
		{
			worker = std::make_unique<worker_t>(args...);
		}
		current_block_context = cbt;
		limits = l;
		initial_slot = i_slot;
		cv.notify_all();
	}
	
	void clear_worker()
	{
		std::lock_guard lock(mtx);
		limits = nullptr;
		current_block_context = nullptr;
		initial_slot = false;
	}

	void totally_clear_worker()
	{
		std::lock_guard lock(mtx);
		worker = nullptr;
		limits = nullptr;
		current_block_context = nullptr;
	}

	using AsyncWorker::wait_for_async_task;
};

template<typename GlobalContext_t, typename BlockContext_t>
class
StaticAsyncWorkerCache
{
	using async_worker_t = AsyncAssemblyWorker<AssemblyWorker<GlobalContext_t, BlockContext_t>>;

	inline static std::vector<std::unique_ptr<async_worker_t>> workers;

public:

	static void resize(uint32_t i)
	{
		while (workers.size() < i) {
			workers.push_back(std::make_unique<async_worker_t>());
		}
	}

	static async_worker_t& get_worker(uint32_t i)
	{
		return *workers.at(i);
	}


	static void
	wait_for_stop_assembly_threads()
	{
		for (auto& worker : workers)
		{
			worker->wait_for_async_task();
			worker->clear_worker();
		}
	}

	static void
	total_reset()
	{
		for (auto& worker : workers)
		{
			worker->wait_for_async_task();
			worker->totally_clear_worker();
		}
	}
};

template<typename GlobalContext_t, typename BlockContext_t>
class
AssemblyWorkerCache 
{
	using worker_t = AssemblyWorker<GlobalContext_t, BlockContext_t>;

	Mempool& mempool;
	GlobalContext_t& global_context;

public:

	AssemblyWorkerCache(Mempool& mp, GlobalContext_t& gc)
		: mempool(mp)
		, global_context(gc)
		{}

	void start_assembly_threads(BlockContext_t* current_block_context, AssemblyLimits* limits, uint32_t n_threads)
	{
		StaticAsyncWorkerCache<GlobalContext_t, BlockContext_t>::resize(n_threads);

		for (uint32_t i = 0; i < n_threads; i++)
		{
			StaticAsyncWorkerCache<GlobalContext_t, BlockContext_t>::get_worker(i)
				.start_worker(current_block_context, limits, true, mempool, global_context);
		}
	}

	void
	wait_for_stop_assembly_threads()
	{
		StaticAsyncWorkerCache<GlobalContext_t, BlockContext_t>::wait_for_stop_assembly_threads();
	}

	~AssemblyWorkerCache()
	{
		StaticAsyncWorkerCache<GlobalContext_t, BlockContext_t>::total_reset();
	}
};


}
