#include "sdk/alloc.h"
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/semaphore.h"
#include "sdk/general_storage.h"
#include "sdk/rpc.h"

#include "erc20.h"

namespace rfq
{

struct rfq_config
{
	sdk::Address wallet_addr;
	sdk::PublicKey external_key;
	sdk::Address external_address;
};

struct rfq_query
{
	sdk::Address tokenA;
	sdk::Address tokenB;
	int64_t amount_A;
};

struct rfq_response_sig_payload
{
	rfq_query q;
	int64_t amount_B;
};

struct rfq_response
{
	sdk::Signature sig;

	int64_t amount_B;
};



constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);

void
write_config(rfq_config const& config)
{
	sdk::set_raw_memory(config_addr, config);
}

auto get_config()
{
	return sdk::get_raw_memory<rfq_config>(config_addr);
}


void make_swap(sdk::Address tokenA, sdk::Address tokenB, int64_t amt_A, int64_t min_recv_B)
{
	auto const& config = get_config();

	rfq_query query
	{
		.tokenA = tokenA,
		.tokenB = tokenB,
		.amount_A = amt_A
	};
	if (amt_A <= 0)
	{
		abort();
	}

	auto response = sdk::external_call<rfq_query, rfq_response>(config.external_address, query);

	rfq_response_sig_payload response_payload
	{
		.q = query,
		.amount_B = response.amount_B	
	};

	if (response.amount_B < min_recv_B)
	{
		abort();
	}

	if (!sdk::check_sig_ed25519(config.external_key, response.sig, response_payload))
	{
		abort();
	}

	erc20::Ierc20 tok_A(tokenA);
	erc20::Ierc20 tok_B(tokenB);

	tok_A.transferFrom(sdk::get_msg_sender(), config.wallet_addr, amt_A);
	tok_B.transferFrom(config.wallet_addr, sdk::get_msg_sender(), response.amount_B);


} 




EXPORT("pub00000000")
initialize()
{
	if (sdk::has_key(config_addr))
	{
		abort();
	}
	auto calldata = sdk::get_calldata<rfq_config>();
	write_config(calldata);
}



}