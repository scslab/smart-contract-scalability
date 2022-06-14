#include "phase/phases.h"

#include "transaction_context/threadlocal_context.h"
#include "state_db/delta_batch.h"
#include "state_db/state_db.h"

#include "contract_db/contract_db.h"

#include "transaction_context/global_context.h"

namespace scs
{

void
phase_merge_delta_batches(DeltaBatch& delta_batch)
{
	delta_batch.merge_in_serial_batches();
		//ThreadlocalContextStore::extract_all_delta_batches());
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


} /* scs */
