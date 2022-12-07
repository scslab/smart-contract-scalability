#pragma once

#include "xdr/rpc.h"

#include <cstdint>

#include <xdrpp/pollset.h>
#include <xdrpp/srpc.h>

namespace scs
{

class EchoServer 
{

public:

	struct Handler
	{
		using rpc_interface_type = ContractRPCV1;

		std::unique_ptr<RpcResult> execute_call(RpcCall const& c) {
			std::printf("found the rpc call\n");
			return std::make_unique<RpcResult>(c.calldata);
		}
	};

	EchoServer(xdr::pollset& ps, const char* port)
		: handler()
		, listener(
			ps,
			xdr::tcp_listen(port, AF_INET),
			false,
			xdr::session_allocator<void>())
		{
			listener.register_service(handler);
		}

private:

	Handler handler;

	xdr::srpc_tcp_listener<> listener;
};

}
