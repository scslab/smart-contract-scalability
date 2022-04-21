
%#include "xdr/types.h"

namespace scs
{

enum TransactionStatus
{
	SUCCESS = 0,
	FAILURE = 1
	//TODO other statuses
};

struct TransactionInvocation
{
	Address invokedAddress;
	uint32 method_name;
	opaque calldata<>;
};

struct Transaction
{
	TransactionInvocation invocation;
	uint64 gas_limit;
	uint64 gas_rate_bid;
};
	
} /* scs */
