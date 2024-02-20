#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include <utils/non_movable.h>

extern "C" {

#include "lfi.h"

}

namespace scs
{


class LFIProc;

class LFIGlobalEngine : public utils::NonMovableOrCopyable
{
	std::mutex mtx;

	struct lfi* lfi_engine;

public:
	LFIGlobalEngine(lfi_syshandler handler);

	~LFIGlobalEngine() {
		lfi_delete(lfi_engine);
	}

	LFIProc new_proc(std::vector<uint8_t> const& program_bytes, void* ctxp);

	void delete_proc(LFIProc& proc);
};

class LFIProc : public utils::NonMovableOrCopyable
{
	struct lfi_proc* proc;

	struct lfi_proc_info info;

	LFIGlobalEngine& main_lfi;

	friend class LFIGlobalEngine;

	LFIProc(struct lfi_proc* proc, struct lfi_proc_info info, LFIGlobalEngine& main_lfi) 
		: proc(proc)
		, info(info)
		, main_lfi(main_lfi)
		{}

public:

	~LFIProc()
	{
		main_lfi.delete_proc(*this);
	};

	void reset_program(std::vector<uint8_t> const& program_bytes);

	void run();

};

}
