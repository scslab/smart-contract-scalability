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
	LFIEngine* lfi_engine;
	uint32_t active_proc_count = 0;

public:
	LFIGlobalEngine(SysHandler handler);

	~LFIGlobalEngine();

	bool
	__attribute__((warn_unused_result))
	new_proc(LFIProc** o_proc, void* ctxp);

	void delete_proc(LFIProc* proc);
};

class DeClProc : public utils::NonMovableOrCopyable
{
	LFIProc* proc;

	LFIProcInfo info;

	void* ctxp;

	LFIGlobalEngine& main_lfi;

	bool actively_running = false;

    uint64_t brkbase = 0;
    size_t brksize = 0;
    size_t brkfull = 0;
    uint64_t base = 0;

public:

	DeClProc(void* ctxp, LFIGlobalEngine& main_lfi);

	~DeClProc();

	operator bool() const {
		return proc != nullptr;
	}

	int
	__attribute__((warn_unused_result))
	run(uint32_t method, std::vector<uint8_t> const& calldata);

	bool 
	__attribute__((warn_unused_result))
	set_program(const uint8_t* bytes, const size_t len);

    void exit(int code);

    uint64_t addr(uint64_t addr);

    uintptr_t sbrk(uint32_t incr);

    bool is_writable(uintptr_t p, uint32_t size) const;
    bool is_readable(uintptr_t p, uint32_t size) const;
};

uint32_t sandboxaddr(uintptr_t p);

}
