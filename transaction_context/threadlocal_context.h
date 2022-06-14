#pragma once

#include <memory>

#include "mtt/utils/threadlocal_cache.h"

#include "state_db/serial_delta_batch.h"
#include "transaction_context/execution_context.h"

#include "transaction_context/threadlocal_types.h"

namespace scs
{

class DeltaBatch;
class WasmContext;

class ThreadlocalContextStore {
	inline static thread_local std::unique_ptr<ExecutionContext> ctx;

	ThreadlocalContextStore() = delete;

public:
	static ExecutionContext& get_exec_ctx();

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
