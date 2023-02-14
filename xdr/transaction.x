/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
