#pragma once

#include "transaction_context/global_context.h"

#include <memory>
#include <vector>

#include "xdr/transaction.h"
#include "xdr/block.h"

namespace scs {

/**
 * Block policy:
 * Header is <hashes of data structures, prev hash, etc>
 * We reject a block if prev_hash doesn't match our hash,
 * but if we execute a block and find no errors
 * but find a hash mismatch at the end,
 * we say that's ok, and move on (presumably rejecting subsequent
 * blocks from that proposer).
 */
class VirtualMachine
{
    GlobalContext global_context;
    std::unique_ptr<BlockContext> current_block_context;

    Hash prev_block_hash;

    bool validate_tx_block(std::vector<Transaction> const& txs);

    void advance_block_number();

  public:
    std::optional<BlockHeader>
    try_exec_tx_block(std::vector<Transaction> const& txs);
};

} // namespace scs
