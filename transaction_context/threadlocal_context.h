#pragma once

#include <memory>

#include "mtt/utils/threadlocal_cache.h"
#include "state_db/serial_delta_batch.h"
#include "transaction_context/execution_context.h"

namespace scs
{

class ExecutionContext;
class WasmContext;

class ThreadlocalContextStore {
	inline static thread_local std::unique_ptr<ExecutionContext> ctx;

	inline static utils::ThreadlocalCache<SerialDeltaBatch> delta_batches;

	ThreadlocalContextStore() = delete;

public:
	static ExecutionContext& get_exec_ctx();
	static SerialDeltaBatch& get_delta_batch();

	using batch_array_t = typename utils::ThreadlocalCache<SerialDeltaBatch>::cache_array_t;

	batch_array_t const& 
	get_all_delta_batches() const
	{
		return delta_batches.get_objects();
	}

	template<typename ...Args>
	static void make_ctx(Args&... args);

	static void post_block_clear();

	// for testing
	static void clear_entire_context();
};

namespace test
{

struct DeferredContextClear
{
	~DeferredContextClear()
	{
		ThreadlocalContextStore::clear_entire_context();
	}
};

} /* test */

} /* scs */
