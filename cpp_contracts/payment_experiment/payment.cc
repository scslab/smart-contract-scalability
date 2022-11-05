#include "erc20.h"

#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/auth_singlekey.h"
#include "sdk/log.h"

namespace payment
{

struct calldata_init
{
    sdk::Address token;
    sdk::PublicKey pk;
};

constexpr static sdk::StorageKey token_addr = sdk::make_static_key(0, 2);

EXPORT("pub00000000")
init()
{
    sdk::Semaphore init_semaphore(sdk::make_static_key(1, 2));
    init_semaphore.acquire();

    auto calldata = sdk::get_calldata<calldata_init>();
    if (sdk::get_raw_memory_opt<sdk::Address>(token_addr))
    {
        abort();
    }

    sdk::set_raw_memory(token_addr, calldata.token);

    sdk::auth_single_pk_register(calldata.pk);

    erc20::Ierc20 token(calldata.token);

    token.allowanceDelta(sdk::get_self(), INT64_MAX);
}

struct calldata_transfer
{
    sdk::Address to;
    int64_t amount;
    uint64_t nonce;
};

EXPORT("pub01000000")
transfer()
{

    auto calldata = sdk::get_calldata<calldata_transfer>();

    sdk::record_self_replay();

    sdk::auth_single_pk_check_sig(0);

    auto token_key = sdk::get_raw_memory<sdk::Address>(token_addr);

    erc20::Ierc20 token(token_key);

    token.transferFrom(sdk::get_self(), calldata.to, calldata.amount);
}

}