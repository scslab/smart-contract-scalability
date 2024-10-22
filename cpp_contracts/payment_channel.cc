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

#include "erc20.h"

#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/auth_singlekey.h"
#include "sdk/log.h"
#include "sdk/flag.h"
#include "sdk/witness.h"

#include <cstdint>

namespace payment_channel
{

enum ChannelState : uint32_t
{
	OPEN = 0,
	CLOSING_BY_ALICE = 1,
	CLOSING_BY_BOB = 2,
	CLOSED = 3
};

struct closing_params
{
	uint64_t seqno;
	int64_t payment_to_alice;
	int64_t payment_to_bob;
};

struct channel_closing_status
{
	uint64_t close_challenge_maxtime;

	ChannelState state;

	closing_params params;
};

struct channel_config
{
	sdk::PublicKey keyAlice;
	sdk::PublicKey keyBob;

	sdk::Address addrAlice;
	sdk::Address addrBob;

	sdk::Address token;

	channel_closing_status status;
};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
static sdk::UniqueFlag alice_claimed(sdk::make_static_key(1, 2));
static sdk::UniqueFlag bob_claimed(sdk::make_static_key(2, 2));
static sdk::UniqueFlag initialized(sdk::make_static_key(3, 2));

channel_config
load_channel_config()
{
	return sdk::get_raw_memory<channel_config>(config_addr);
}

void
write_channel_config(const channel_config& config)
{
	sdk::set_raw_memory(config_addr, config);
}

void mutual_closing(
	const sdk::Signature& sigAlice, 
	const sdk::Signature& sigBob, 
	int64_t payment_to_alice, 
	int64_t payment_to_bob)
{
	sdk::Hash tx_hash = sdk::get_invoked_hash();
	auto config = load_channel_config();

	if (!sdk::check_sig_ed25519(config.keyAlice, sigAlice, tx_hash))
	{
		abort();
	}
	if (!sdk::check_sig_ed25519(config.keyBob, sigBob, tx_hash))
	{
		abort();
	}

	erc20::Ierc20 token(config.token);

	token.transferFrom(sdk::get_self(), config.addrAlice, payment_to_alice);
	token.transferFrom(sdk::get_self(), config.addrBob, payment_to_bob);
}

void require_state(ChannelState state, const channel_config& config)
{
	if (state != config.status.state)
	{
		abort();
	}
}

void
alice_init_close(
	const sdk::Signature& alice_sig_on_tx,
	const sdk::Signature& bob_sig_on_closing_params,
	const closing_params& closing_params)
{
	auto config = load_channel_config();

	if (!sdk::check_sig_ed25519(config.keyAlice, alice_sig_on_tx, sdk::get_invoked_hash()))
	{
		abort();
	}
	if (!sdk::check_sig_ed25519(config.keyBob, bob_sig_on_closing_params, closing_params))
	{
		abort();
	}

	require_state(ChannelState::OPEN, config);

	config.status.params = closing_params;
	config.status.state = ChannelState::CLOSING_BY_ALICE;
	config.status.close_challenge_maxtime = sdk::get_block_number() + 1000;

	write_channel_config(config);
}

void
bob_init_close(
	const sdk::Signature& alice_sig_on_closing_params,
	const sdk::Signature& bob_sig_on_tx,
	const closing_params& closing_params)
{
	auto config = load_channel_config();

	if (!sdk::check_sig_ed25519(config.keyBob, bob_sig_on_tx, sdk::get_invoked_hash()))
	{
		abort();
	}
	if (!sdk::check_sig_ed25519(config.keyAlice, alice_sig_on_closing_params, closing_params))
	{
		abort();
	}

	require_state(ChannelState::OPEN, config);

	config.status.params = closing_params;
	config.status.state = ChannelState::CLOSING_BY_BOB;
	config.status.close_challenge_maxtime = sdk::get_block_number() + 1000;

	write_channel_config(config);
}

void finish_closing(channel_config& config)
{
	if (config.status.state == ChannelState::CLOSED)
	{
		return;
	}
	if (config.status.state == ChannelState::OPEN)
	{
		abort();
	}

	if (sdk::get_block_number() < config.status.close_challenge_maxtime)
	{
		abort();
	}

	config.status.state = ChannelState::CLOSED;
	write_channel_config(config);
}


void alice_claim_closed()
{
	auto config = load_channel_config();
	finish_closing(config);

	erc20::Ierc20 token(config.token);

	alice_claimed.acquire();

	token.transferFrom(sdk::get_self(), config.addrAlice, config.status.params.payment_to_alice);
}

void bob_claim_closed()
{
	auto config = load_channel_config();
	finish_closing(config);

	erc20::Ierc20 token(config.token);

	bob_claimed.acquire();

	token.transferFrom(sdk::get_self(), config.addrBob, config.status.params.payment_to_bob);
}

void bob_challenge_close(
	const sdk::Signature& alice_sig_on_closing_params,
	const sdk::Signature& bob_sig_on_closing_params,
	const closing_params& closing_params)
{
	auto config = load_channel_config();

	if (!sdk::check_sig_ed25519(config.keyBob, bob_sig_on_closing_params, closing_params))
	{
		abort();
	}
	if (!sdk::check_sig_ed25519(config.keyAlice, alice_sig_on_closing_params, closing_params))
	{
		abort();
	}

	require_state(ChannelState::CLOSING_BY_ALICE, config);
	
	if (closing_params.seqno > config.status.params.seqno)
	{
		// bob challenged with a higher seqno

		erc20::Ierc20 token(config.token);

		token.transferFrom(sdk::get_self(), config.addrBob, closing_params.payment_to_alice + closing_params.payment_to_bob);

		/** optional: clean up state here 
		 * no money left, so rest of state doesn't matter
		 */

		return;
	}
	abort();
}

void alice_challenge_close(
	const sdk::Signature& alice_sig_on_closing_params,
	const sdk::Signature& bob_sig_on_closing_params,
	const closing_params& closing_params)
{
	auto config = load_channel_config();

	if (!sdk::check_sig_ed25519(config.keyBob, bob_sig_on_closing_params, closing_params))
	{
		abort();
	}
	if (!sdk::check_sig_ed25519(config.keyAlice, alice_sig_on_closing_params, closing_params))
	{
		abort();
	}

	require_state(ChannelState::CLOSING_BY_BOB, config);
	
	if (closing_params.seqno > config.status.params.seqno)
	{
		// bob challenged with a higher seqno

		erc20::Ierc20 token(config.token);

		token.transferFrom(sdk::get_self(), config.addrAlice, closing_params.payment_to_alice + closing_params.payment_to_bob);

		/* optional: clean up state here */
		return;
	}
	abort();
}

struct calldata_ctor
{
	sdk::PublicKey keyAlice;
	sdk::PublicKey keyBob;

	sdk::Address addrAlice;
	sdk::Address addrBob;

	sdk::Address token;

	int64_t alice_amount;
	int64_t bob_amount;
};

EXPORT("pub00000000")
initialize()
{
	auto calldata = sdk::get_calldata<calldata_ctor>();

	initialized.acquire();

    channel_config config {
    	.keyAlice = calldata.keyAlice,
    	.keyBob = calldata.keyBob,
    	.addrAlice = calldata.addrAlice,
    	.addrBob = calldata.addrBob,
    	.token = calldata.token,
    	.status = channel_closing_status {
    		.close_challenge_maxtime = 0,
    		.state = ChannelState::OPEN,
    		.params = closing_params {
    			.seqno = 0,
    			.payment_to_alice = 0,
    			.payment_to_bob = 0
    		}
    	}
    };

    write_channel_config(config);

    erc20::Ierc20 token(calldata.token);

    token.transferFrom(calldata.addrAlice, sdk::get_self(), calldata.alice_amount);
    token.transferFrom(calldata.addrBob, sdk::get_self(), calldata.bob_amount);
    return 0;
}

struct calldata_mutual_close
{
	int64_t payment_alice;
	int64_t payment_bob;
};

EXPORT("pub01000000")
pub_mutual_close()
{
	auto calldata = sdk::get_calldata<calldata_mutual_close>();

	sdk::Signature aliceSig = sdk::get_witness<sdk::Signature>(1);
	sdk::Signature bobSig = sdk::get_witness<sdk::Signature>(2);

	mutual_closing(aliceSig, bobSig, calldata.payment_alice, calldata.payment_bob);
	return 0;
}

struct calldata_closing
{
	closing_params params;
	sdk::Signature alice_sig;
	sdk::Signature bob_sig;
};

EXPORT("pub02000000")
pub_alice_init_closing()
{
	auto calldata = sdk::get_calldata<calldata_closing>();

	alice_init_close(calldata.alice_sig, calldata.bob_sig, calldata.params);
	return 0;
}

EXPORT("pub03000000")
pub_bob_init_closing()
{
	auto calldata = sdk::get_calldata<calldata_closing>();

	bob_init_close(calldata.alice_sig, calldata.bob_sig, calldata.params);
	return 0;
}

EXPORT("pub04000000")
pub_alice_challenge_close()
{
	auto calldata = sdk::get_calldata<calldata_closing>();
	alice_challenge_close(calldata.alice_sig, calldata.bob_sig, calldata.params);
	return 0;
}

EXPORT("pub05000000")
pub_bob_challenge_close()
{
	auto calldata = sdk::get_calldata<calldata_closing>();
	bob_challenge_close(calldata.alice_sig, calldata.bob_sig, calldata.params);
	return 0;
}


EXPORT("pub06000000")
pub_alice_finish_closing()
{
	alice_claim_closed();
	return 0;
}

EXPORT("pub07000000")
pub_bob_finish_closing()
{
	bob_claim_closed();
	return 0;
}





}