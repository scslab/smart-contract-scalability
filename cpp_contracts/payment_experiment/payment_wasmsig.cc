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
#include "sdk/log.h"
#include "sdk/auth_singlekey_wasmed25519.h"

namespace payment
{

struct calldata_init
{
    sdk::Address token;
    sdk::PublicKey pk;
    uint16_t size_increase;
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

    sdk::wasm_auth_single_pk_register(calldata.pk);

    erc20::Ierc20 token(calldata.token);

    token.allowanceDelta(sdk::get_self(), INT64_MAX);

    sdk::replay_cache_size_increase(calldata.size_increase);
    return 0;
}

struct calldata_transfer
{
    sdk::Address to;
    int64_t amount;
    uint64_t nonce;
    uint64_t expiration_time;
};

EXPORT("pub01000000")
transfer()
{
    auto calldata = sdk::get_calldata<calldata_transfer>();

    sdk::record_self_replay(calldata.expiration_time);

    sdk::wasm_auth_single_pk_check_sig(0);

    auto token_key = sdk::get_raw_memory<sdk::Address>(token_addr);

    erc20::Ierc20 token(token_key);

    token.transferFrom(sdk::get_self(), calldata.to, calldata.amount);
    return 0;
}

}
