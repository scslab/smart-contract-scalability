%#include "xdr/types.h"
%#include "xdr/transaction.h"

namespace scs
{

struct Block
{
	TxSetEntry transactions<>;
};

struct BlockHeader
{
	uint64 block_number;
	Hash prev_header_hash;

	Hash tx_set_hash;
	Hash modified_keys_hash;
	Hash state_db_hash;
	Hash contract_db_hash;
};

} /* namespace scs */
