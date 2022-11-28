
%#include "xdr/types.h"

%#include "xdr/rpc.h"

namespace scs
{

enum TransactionStatus
{
	SUCCESS = 0,
	FAILURE = 1
	//TODO other statuses
};

// validity check: flip bit <phase_num> if you fail
// validation in phase <phase_num>.
// valid in phase (phase_num) if (validity bits) <= phase_enum
enum TransactionFailurePoint
{
	COMPUTE          = 0x40000000,
	CONFLICT_PHASE_1 = 0x20000000,
	FINAL            = 0
};

struct TransactionInvocation
{
	Address invokedAddress;
	uint32 method_name;
	opaque calldata<>;
};

% static_assert(sizeof(Address) == sizeof(PublicKey), "pk size should be addr size");

struct Transaction
{
	TransactionInvocation invocation;
	uint64 gas_limit;
	uint64 gas_rate_bid;

	Contract contracts_to_deploy<>;
};

struct WitnessEntry
{
	uint64 key;
	opaque value<>;
};

struct SignedTransaction
{
	Transaction tx;
	WitnessEntry witnesses<>;
};

struct NondeterministicResults
{
	RpcResult rpc_results<>;
};

struct TxSetEntry
{
	SignedTransaction tx;
	NondeterministicResults nondeterministic_results<>;
};

struct TransactionResults
{
	NondeterministicResults ndeterministic_results;
	TransactionLog logs<>;
};
	
} /* scs */
