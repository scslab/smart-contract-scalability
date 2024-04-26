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
	struct lfi* lfi_engine;
	uint32_t active_proc_count = 0;

public:
	LFIGlobalEngine(lfi_syshandler handler);

	~LFIGlobalEngine();

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

    uint64_t brkbase = 0;
    size_t brksize = 0;
    uint64_t base = 0;

public:

	LFIProc(void* ctxp, LFIGlobalEngine& main_lfi);

	~LFIProc();

	operator bool() const {
		return proc != nullptr;
	}

	int
	__attribute__((warn_unused_result))
	run(uint32_t method, std::vector<uint8_t> const& calldata);

	int 
	__attribute__((warn_unused_result))
	set_program(RunnableScriptView const& script);

    void exit [[noreturn]] (int code);

    uint64_t addr(uint64_t addr);

    uintptr_t sbrk(uint32_t incr);

    bool is_writable(uintptr_t p, uint32_t size) const;
    bool is_readable(uintptr_t p, uint32_t size) const;
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
