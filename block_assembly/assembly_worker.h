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
class LFIGlobalEngine;

template<typename GlobalContext_t, typename BlockContext_t>
class AssemblyWorker
{
	Mempool& mempool;
	GlobalContext_t& global_context;
	BlockContext_t& block_context;
	AssemblyLimits& limits;
	ExecutionContext<TransactionContext<GlobalContext_t>> exec_ctx;

public:

	AssemblyWorker(Mempool& mempool, GlobalContext_t& global_context, BlockContext_t& block_context, AssemblyLimits& limits, LFIGlobalEngine& engine)
		: mempool(mempool)
		, global_context(global_context)
		, block_context(block_context)
		, limits(limits)
		, exec_ctx(engine)
		{
		}

	void run();
};

template<typename worker_t>
class AsyncAssemblyWorker : public utils::AsyncWorker
{
	worker_t worker;
	bool running = false;

	bool exists_work_to_do() override final
	{
		return running;
	}

	void run();

public:

	template<typename... Args>
	AsyncAssemblyWorker(Args&... args)
		: utils::AsyncWorker()
		, worker(args...)
	{
		start_async_thread(
			[this] {run();}
		);
	}

	~AsyncAssemblyWorker()
	{
		terminate_worker();
	}

	void start_worker()
	{
		std::lock_guard lock(mtx);
		running = true;
		cv.notify_all();
	}

	using AsyncWorker::wait_for_async_task;
};

template<typename GlobalContext_t, typename BlockContext_t>
class
StaticAssemblyWorkerCache 
{
	using worker_t = AssemblyWorker<GlobalContext_t, BlockContext_t>;

	inline static std::vector<std::unique_ptr<AsyncAssemblyWorker<worker_t>>> workers;
	inline static std::mutex mtx;

	StaticAssemblyWorkerCache() = delete;

public:

	static void start_assembly_threads(Mempool& mp, GlobalContext_t& gc, BlockContext_t& bc, AssemblyLimits& limits, LFIGlobalEngine& engine, uint32_t n_threads)
	{
		std::lock_guard lock(mtx);
		while(workers.size() < n_threads)
		{
			workers.push_back(std::make_unique<AsyncAssemblyWorker<worker_t>>(mp, gc, bc, limits, engine));
		}

		for (uint32_t i = 0; i < n_threads; i++)
		{
			workers[i] -> start_worker();
		}
	}

	static void
	wait_for_stop_assembly_threads()
	{
		std::lock_guard lock(mtx);

		for (auto& worker : workers)
		{
			worker -> wait_for_async_task();
		}
	}
};


}