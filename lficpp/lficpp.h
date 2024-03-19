#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <stdexcept>
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
	new_proc(struct lfi_proc** o_proc, void* ctxp);

	void delete_proc(struct lfi_proc* proc);
};

class LFIProc : public utils::NonMovableOrCopyable
{
	struct lfi_proc* proc;

	struct lfi_proc_info info;

	void* ctxp;

	LFIGlobalEngine& main_lfi;

	bool actively_running = false;

public:

	LFIProc(void* ctxp, LFIGlobalEngine& main_lfi);

	~LFIProc();

	operator bool() const {
		return proc != nullptr;
	}

	int
	__attribute__((warn_unused_result))
	run();

	int 
	__attribute__((warn_unused_result))
	set_program(const uint8_t* bytes, const size_t len);

	void exit_error();
};

}
