#pragma once

#include <optional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>

#include <utils/async_worker.h>

namespace scs
{

class AssemblyLimits;
class Mempool;
class GlobalContext;
class BlockContext;

class AssemblyWorker
{
	Mempool& mempool;
	GlobalContext& global_context;
	BlockContext& block_context;
	AssemblyLimits& limits;

public:

	AssemblyWorker(Mempool& mempool, GlobalContext& global_context, BlockContext& block_context, AssemblyLimits& limits)
		: mempool(mempool)
		, global_context(global_context)
		, block_context(block_context)
		, limits(limits)
		{
		}

	void run();
};

class AsyncAssemblyWorker : public utils::AsyncWorker
{
	std::optional<AssemblyWorker> worker;

	bool exists_work_to_do() override final
	{
		return worker.has_value();
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

	void start_worker(AssemblyWorker w)
	{
		std::lock_guard lock(mtx);
		worker.emplace(w);
		cv.notify_all();
	}

	using AsyncWorker::wait_for_async_task;
};

class
StaticAssemblyWorkerCache 
{
	inline static std::vector<std::unique_ptr<AsyncAssemblyWorker>> workers;
	inline static std::mutex mtx;

	StaticAssemblyWorkerCache() = delete;

public:

	static void start_assembly_threads(Mempool& mp, GlobalContext& gc, BlockContext& bc, AssemblyLimits& limits, uint32_t n_threads)
	{
		std::lock_guard lock(mtx);
		while(workers.size() < n_threads)
		{
			workers.push_back(std::make_unique<AsyncAssemblyWorker>());
		}

		for (uint32_t i = 0; i < n_threads; i++)
		{
			workers[i] -> start_worker(AssemblyWorker(mp, gc, bc, limits));
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