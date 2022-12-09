#include "phase/phases.h"

#include "threadlocal/threadlocal_context.h"

#include "state_db/state_db.h"

#include "contract_db/contract_db.h"

#include "transaction_context/global_context.h"

#include <utils/time.h>
#include <tbb/task_group.h>

namespace scs
{

void phase_finish_block(GlobalContext& global_structures, BlockContext& block_structures)
{
	auto ts = utils::init_time_measurement();
	tbb::task_group g;
	g.run([&] () {
		block_structures.tx_set.finalize();
	});
	//block_structures.tx_set.finalize();
	block_structures.modified_keys_list.merge_logs();
	std::printf("keylist merge %lf\n", utils::measure_time(ts));
	global_structures.contract_db.commit();
	std::printf("contract db commit %lf\n", utils::measure_time(ts));
	global_structures.state_db.commit_modifications(block_structures.modified_keys_list);
	std::printf("commit statedb %lf\n", utils::measure_time(ts));
	g.wait();
	std::printf("task group wait %lf\n", utils::measure_time(ts));
	ThreadlocalContextStore::post_block_clear();
	std::printf("post block tlcs clear %lf\n", utils::measure_time(ts));
}

void phase_undo_block(GlobalContext& global_structures, BlockContext& block_structures)
{
	block_structures.modified_keys_list.merge_logs();

	global_structures.contract_db.rewind();
	global_structures.state_db.rewind_modifications(block_structures.modified_keys_list);

	ThreadlocalContextStore::post_block_clear();
}

/*
void
phase_merge_delta_batches(DeltaBatch& delta_batch)
{
	delta_batch.merge_in_serial_batches();
}

void
phase_filter_deltas(GlobalContext const& global_structures, DeltaBatch& delta_batch, TxBlock& tx_block)
{
	global_structures.state_db.populate_delta_batch(delta_batch);

	delta_batch.filter_invalid_deltas(tx_block);
}

void
phase_compute_state_updates(DeltaBatch& delta_batch, const TxBlock& tx_block)
{
	delta_batch.apply_valid_deltas(tx_block);
}

void 
phase_finish_block(GlobalContext& global_structures, const DeltaBatch& delta_batch, const TxBlock& tx_block)
{
	global_structures.contract_db.commit(tx_block);
	global_structures.state_db.apply_delta_batch(delta_batch);
	ThreadlocalContextStore::post_block_clear();
}
*/


} /* scs */
