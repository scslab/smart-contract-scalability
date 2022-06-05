#include "state_db/delta_batch.h"
#include "state_db/delta_vec.h"
#include "state_db/object_mutator.h"
#include "state_db/serial_delta_batch.h"

#include "tx_block/tx_block.h"

namespace scs
{

void 
DeltaBatch::merge_in_serial_batch(SerialDeltaBatch& batch)
{
	throw std::runtime_error("incorrect implementation");
	auto& m = batch.get_delta_map();
	deltas.insert(m.begin(), m.end());
}

void 
DeltaBatch::filter_invalid_deltas(TxBlock& txs) {
	for (auto& [_, v] : deltas)
	{
		v.second.filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(v.first, txs);
	}
}

void 
DeltaBatch::apply_valid_deltas(TxBlock const& txs) {
	for (auto& [_, v] : deltas)
	{
		v.second.apply_valid_deltas(v.first, txs);
	}
}

} /* scs */
