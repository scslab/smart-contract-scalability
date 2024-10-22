#pragma once

#include <cstdint>

#include "sdk/macros.h"
#include "../common/syscall_nos.h"

namespace sdk
{

namespace detail
{

BUILTIN("syscall")
uint64_t
_builtin_syscall(
	uint64_t callno,
	uint64_t arg0,
	uint64_t arg1, 
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5);

uint64_t 
builtin_syscall(
	SYSCALLS syscall,
	uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5) {
	return _builtin_syscall(
		static_cast<uint64_t>(syscall), 
		arg0,
		arg1,
		arg2,
		arg3,
		arg4,
		arg5);

}

}


}

