
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
	opaque methodName<>;
	opaque calldata<>;
};
	
} /* scs */
