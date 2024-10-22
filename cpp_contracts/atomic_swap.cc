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

/**
 * 
 * This is a regular atomic swap (crosschain) system.
 * 
 * Alice posts collateral A, says transferrable to B if B reveals x (commit to H(x)) alice knows x
 * Bob posts collateral B, says transferrable to A if A reveals x (commit to H(x), which bob knows from Alice's tx)
 * Alice reveals x, claims her money
 * Bob claims money
 * 
 **/

#include "sdk/log.h"
#include "sdk/raw_memory.h"
#include "sdk/flag.h"
#include "sdk/types.h"
#include "sdk/calldata.h"
#include "sdk/constexpr.h"
#include "sdk/crypto.h"

#include "erc20.h"

namespace atomic_swap
{

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
static sdk::UniqueFlag closed(sdk::make_static_key(1, 2));
static sdk::UniqueFlag initialized(sdk::make_static_key(2, 2));

struct atomic_swap_config
{
	sdk::Address source_addr;
	sdk::Address recipient_addr;

	sdk::Address token;
	int64_t amount;

	sdk::Hash commitment;

	uint64_t expiry_time;
};

atomic_swap_config
load_config()
{
	return sdk::get_raw_memory<atomic_swap_config>(config_addr);
}

void
write_config(const atomic_swap_config& config)
{
	sdk::set_raw_memory(config_addr, config);
}

void reveal_preimage(sdk::Hash const& preimage, const atomic_swap_config& config)
{
	auto h = sdk::hash(preimage);
	if (h != config.commitment)
	{
		abort();
	}
	sdk::log(preimage);
}

void
claim_swap(sdk::Hash const& preimage)
{
	auto config = load_config();
	reveal_preimage(preimage, config);

	erc20::Ierc20 token(config.token);

	token.transferFrom(sdk::get_self(), config.recipient_addr, config.amount);

	closed.acquire();
}



EXPORT("pub00000000")
initialize()
{
	auto config = sdk::get_calldata<atomic_swap_config>();
	write_config(config);
	initialized.acquire();
	return 0;
}


struct calldata_claim
{
	sdk::Hash preimage;
};

EXPORT("pub01000000")
claim_preimage()
{
	auto calldata = sdk::get_calldata<calldata_claim>();
	claim_swap(calldata.preimage);
	return 0;
}

EXPORT("pub02000000")
reclaim_expired()
{
	auto config = load_config();

	if (config.expiry_time < sdk::get_block_number())
	{
		abort();
	}

	erc20::Ierc20 token(config.token);
	token.transferFrom(sdk::get_self(), config.source_addr, config.amount);

	closed.acquire();
	return 0;
}


}


