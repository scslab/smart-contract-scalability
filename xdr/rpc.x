%#include "xdr/types.h"

namespace scs
{
	
struct RpcCall
{
	Hash target;
	opaque calldata<>;
};

struct RpcResult
{
	opaque result<>;
};

program ContractRPC {
	version ContractRPCV1 {
		RpcResult execute_call(RpcCall) = 1;
	} = 1;
} = 0x11111111;

}
