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

#pragma once

#include "sdk/types.h"
#include "sdk/invoke.h"

namespace erc721
{

struct calldata_approve
{
	sdk::Hash nft;
	sdk::Address approved;
};

struct calldata_approveAll
{
	sdk::Address op;
	bool auth;
};

struct calldata_transferFrom
{
	sdk::Address from;
	sdk::Address to;
	sdk::Hash nft;
};

class Ierc721
{
    const sdk::Address& addr;

public:

    constexpr 
    Ierc721(const sdk::Address& addr)
        : addr(addr)
        {}

    inline void
    approve(sdk::Hash const& nft, sdk::Address const& approved)
    {
    	approve(
    		calldata_approve {
    			.nft = nft,
    			.approved = approved
    		});
    }

    inline void 
    approve(calldata_approve const& calldata)
    {
    	sdk::invoke(addr, 0, calldata);
    }

    inline void
    approveAll(sdk::Address const& op, bool auth)
    {
    	approveAll(
    		calldata_approveAll {
    			.op = op,
    			.auth = auth
    		});
    }

    inline void 
    approveAll(calldata_approveAll const& calldata)
    {
    	sdk::invoke(addr, 1, calldata);
    }

   	inline void
    transferFrom(sdk::Address const& from, sdk::Address const& to, sdk::Hash const& nft)
    {
    	transferFrom(
    		calldata_transferFrom {
    			.from = from,
    			.to = to,
    			.nft = nft
    		});
    }

    inline void 
    transferFrom(calldata_transferFrom const& calldata)
    {
    	sdk::invoke(addr, 2, calldata);
    }
};


}