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
