#pragma once

#include <memory>

namespace scs {

class DeltaTypeClass;
class DeltaVector;
class TxBlock;

class FilterContext
{
    bool check_valid_dtc();
    void invalidate_entire_vector(DeltaVector const& v, TxBlock& txs);

    virtual void add_vec_when_tc_valid(DeltaVector const& v) = 0;

    virtual void prune_invalid_txs_when_tc_valid(DeltaVector const& v,
                                                 TxBlock& txs)
        = 0;

  protected:
    FilterContext(DeltaTypeClass const& dtc);

    DeltaTypeClass const& dtc;

  public:
    /**
     * Workflow:
     * 		- call add_vec on each DeltaVector on the object.
     * 		  This accumulates the list of side effects for the deltas.
     * 		- Call prune_invalid_txs on each DV.
     * 		  If something failed during validation, this logs txs as failed
     * 		  to the txblock.
     */

    void add_vec(DeltaVector const& v);
    void prune_invalid_txs(DeltaVector const& v, TxBlock& txs);
};

std::unique_ptr<FilterContext>
make_filter_context(DeltaTypeClass const& dtc);

} // namespace scs
