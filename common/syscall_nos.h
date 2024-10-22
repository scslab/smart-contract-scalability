#pragma once

#include <cstdint>

enum class SYSCALLS : uint64_t
{
	// standard syscalls, not groundhog-specific
	EXIT = 500,
	WRITE = 501,
	WRITE_BYTES = 502,

	// groundhog runtime
	LOG = 600,
	INVOKE = 601,
	GET_SENDER = 602,
	GET_SELF_ADDR = 603,
	GET_SRC_TX_HASH = 604,
	GET_INVOKED_TX_HASH = 605,
	GET_BLOCK_NUMBER = 606,

	RETURN = 626,
	GET_CALLDATA = 627,
	GET_CALLDATA_LEN = 628,

	GAS = 629,

	// storage fns
	HAS_KEY = 607,

	// raw memory
	RAW_MEM_SET = 608,
	RAW_MEM_GET = 609,
	RAW_MEM_GET_LEN = 610,

	// delete
	DELETE_KEY_LAST = 611,

	// nonnegative int
	NNINT_SET_ADD = 612,
	NNINT_ADD = 613,
	NNINT_GET = 614,

	// hashset
	HS_INSERT = 615,
	HS_INC_LIMIT = 616,
	HS_CLEAR = 617,
	HS_GET_SIZE = 618,
	HS_GET_MAX_SIZE = 619,
	HS_GET_INDEX_OF = 620,
	HS_GET_INDEX = 621,

	// contracts
	CONTRACT_CREATE = 622,
	CONTRACT_DEPLOY = 623,

	// witnesses
	WITNESS_GET = 624,
	WITNESS_GET_LEN = 625,

	// crypto
	HASH = 700,
	VERIFY_ED25519 = 701,
};

