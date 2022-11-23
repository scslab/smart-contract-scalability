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