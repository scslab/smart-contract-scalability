#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <coroutine>


#include <utils/non_movable.h>

extern "C" {

#include "lfi.h"

}

namespace scs
{

class LFIGlobalEngine : public utils::NonMovableOrCopyable
{
	std::mutex mtx;

	struct lfi* lfi_engine;

public:
	LFIGlobalEngine(lfi_syshandler handler);

	~LFIGlobalEngine() {
		lfi_delete(lfi_engine);
	}

	int
	__attribute__((warn_unused_result))
	new_proc(struct lfi_proc** o_proc, struct lfi_proc_info* o_proc_info, std::vector<uint8_t> const& program_bytes, void* ctxp);

	void delete_proc(struct lfi_proc* proc);
};

struct LFIMessage {
	std::vector<uint8_t> bytes;
};

class LFIProc : public utils::NonMovableOrCopyable
{
	struct lfi_proc* proc;

	struct lfi_proc_info info;

	void* ctxp;

	LFIGlobalEngine& main_lfi;

public:

	LFIProc(void* ctxp, LFIGlobalEngine& main_lfi) 
		: proc(nullptr)
		, info()
		, ctxp(ctxp)
		, main_lfi(main_lfi)
		{}


	~LFIProc();

	int 
	__attribute__((warn_unused_result))
	set_program(std::vector<uint8_t> const& program_bytes);

	generator<LFIMessage> run(LFIMessage const& input);
};

}
