#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <utils/non_movable.h>
#include <memory>

#include "xdr/types.h"
#include "contract_db/runnable_script.h"

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

	// returns gas consumed
	uint64_t
	run(uint32_t method, std::vector<uint8_t> const& calldata, uint32_t gas_limit);

	bool
	__attribute__((warn_unused_result))
	set_program(RunnableScriptView const& script);

    void exit [[noreturn]] (int code);

    uint64_t addr(uint64_t addr);

    uintptr_t sbrk(uint32_t incr);

    bool is_writable(uintptr_t p, uint32_t size) const;
    bool is_readable(uintptr_t p, uint32_t size) const;

    void deduct_gas(uint64_t deduction);
    uint64_t get_available_gas();
};

uint32_t sandboxaddr(uintptr_t p);

class LFIContract : public utils::NonMovableOrCopyable
{
    std::shared_ptr<const Contract> base;

  public:
    LFIContract(std::shared_ptr<const Contract> unmetered);

    operator bool() const { return (bool) base; }

    RunnableScriptView to_view() const;
    Hash hash() const;
};

}
