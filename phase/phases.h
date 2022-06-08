#pragma once

namespace scs
{

class DeltaBatch;
class GlobalContext;
class TxBlock;

void
phase_merge_delta_batches(DeltaBatch& delta_batch);

void
phase_filter_deltas(GlobalContext const& global_structures, DeltaBatch& delta_batch, TxBlock& tx_block);

void
phase_compute_state_updates(DeltaBatch& delta_batch, const TxBlock& tx_block);

void
phase_finish_block(GlobalContext& global_structures, const DeltaBatch& delta_batch, const TxBlock& tx_block);
} /* scs */
