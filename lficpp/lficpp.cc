#include "lficpp/lficpp.h"

#include <utility>

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

#include "crypto/hash.h"
#include "transaction_context/error.h"

#include "crypto/hash.h"

namespace scs
{
	
LFIGlobalEngine::LFIGlobalEngine(SysHandler handler)
	: lfi_engine(nullptr)
	{
		LFIOptions opts = (LFIOptions) {
			.noverify = 1, // programs should be verified when they're registered, not at runtime
            .poc = 1,
            .sysexternal = true,
			.pagesize = (size_t) getpagesize(), // TODO: this should probably be 16k hardcoded
			.stacksize = 65536, 
			.gas = 0,
            .syshandler = handler,
		};

		lfi_engine = lfi_new(opts);
        assert(lfi_engine);

		std::printf("add vaspaces\n");
        bool ok = lfi_reserve(lfi_engine,  8ULL * 1024  * 1024 * 1024 * 1024); // 8TB
        assert(ok);
	}

LFIGlobalEngine::~LFIGlobalEngine() {
	lfi_delete(lfi_engine);
	if (active_proc_count != 0)
	{
		std::printf("invalid lfi engine shutdown!");
		std::fflush(stdout);
		std::terminate();
	}
}


bool 
LFIGlobalEngine::new_proc(LFIProc** o_proc, void* ctxp)
{
	std::lock_guard lock(mtx);
	active_proc_count++;
	return lfi_addproc(lfi_engine, o_proc, ctxp);
}

void 
LFIGlobalEngine::delete_proc(LFIProc* proc)
{
	std::lock_guard lock(mtx);
	lfi_rmproc(lfi_engine, proc);
	active_proc_count--;
}

DeClProc::DeClProc(void* ctxp, LFIGlobalEngine& main_lfi) 
	: proc(nullptr)
	, info()
	, ctxp(ctxp)
	, main_lfi(main_lfi)
{
	if (!main_lfi.new_proc(&proc, ctxp))
	{
		proc = nullptr;
	} else {
        base = lfi_proc_base(proc);
    }
}

uint64_t
DeClProc::addr(uint64_t a) {
    return base | ((uint32_t) a);
}

bool 
DeClProc::set_program(RunnableScriptView const& script)
{
	if (actively_running) {
		std::printf("wtf\n");
		return -1;
	}
	if (script.len == 0) {
		if (script.data != NULL) {
			perror("invalid contract in db");
			std::abort();
		}
		std::printf("len=0\n");
		return -1;
	}
	return lfi_proc_loadelf(proc, const_cast<uint8_t*>(script.data), script.len, NULL, 0, &info);
}

uint32_t sandboxaddr(uintptr_t realaddr) {
	return realaddr & 0xFFFF'FFFF;
}

uint64_t
DeClProc::run(uint32_t method, std::vector<uint8_t> const& calldata, uint32_t gas)
{
	if (actively_running) {
		throw HostError("reentrance in DeClProc");
	}
	lfi_proc_init(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);

	brkbase = info.lastva;
	brksize = 0;

	LFIRegs* regs = lfi_proc_regs(proc);
	regs -> x0 = method;
	
	if (sbrk(calldata.size()) == static_cast<uintptr_t>(-1)) {
		throw HostError("failed to allocate calldata");
	}

	std::memcpy((void*)brkbase, calldata.data(), calldata.size());

	regs -> x1 = sandboxaddr(brkbase);
	regs -> x2 = calldata.size();

	regs -> x23 = sandboxaddr(gas);

	actively_running = true;
	int code = lfi_proc_start(proc);
	actively_running = false;

	if (code != 0) {
		throw HostError("lfi_proc_start returned errorcode");
	}

	if (brksize != 0)
	{
		if (munmap((void*) brkbase, brksize) != 0) {
			std::printf("cleanup failed\n");
			std::abort();
		}
	}

	uint64_t remaining_gas = regs -> x23;
	if (remaining_gas > gas) {
		throw HostError("gas usage exceeded limit");
	}
	return gas - remaining_gas;
}

void
DeClProc::deduct_gas(uint64_t gas) {
	LFIRegs* regs = lfi_proc_regs(proc);
	if (gas > regs -> x23) {
		throw HostError("gas usage exceeded limit in deduct_gas");
	}
	regs -> x23 -= gas;
}
uint64_t 
DeClProc::get_available_gas() {
	LFIRegs* regs = lfi_proc_regs(proc);
	return regs -> x23;
}

void 
DeClProc::exit(int code) {
	if (!actively_running) {
		std::printf("bizarre\n");
		std::abort();
	}
	actively_running = false;
	lfi_proc_exit(code);
	std::unreachable();
}

DeClProc::~DeClProc()
{
	if (actively_running) {
		//impossible
		std::terminate();
	}
	if (proc != nullptr)
	{
		main_lfi.delete_proc(proc);
	}
}

uintptr_t
DeClProc::sbrk(uint32_t incr)
{
	
	uintptr_t brkp = brkbase + brksize;
    	if (incr == 0) {
		return brkp;
	}
	if (brkp + incr < base + (4ULL * 1024 * 1024 * 1024)) {
        void* map;
        if (brksize == 0) {
            map = mmap((void*) brkbase, brksize + incr, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        } else {
            map = mremap((void*) brkbase, brksize, brksize + incr, 0);
        }
        if (map == (void*) -1) {
            perror("sbrk: mmap");
            std::abort();
        }
        brksize += incr;
    } else
    {
    	brkp = -1;
    }

    return brkp;
}

bool 
DeClProc::is_writable(uintptr_t p, uint32_t size) const
{
	if (p < brkbase) {
		return false;
	}
	if (p + size > brkbase + brksize) {
		return false;
	}
	return true;
}

bool
DeClProc::is_readable(uint64_t p, uint32_t size) const {
	return is_writable(p, size);
}

LFIContract::LFIContract(std::shared_ptr<const Contract> unmetered)
	: base(unmetered)
	{
		std::printf("VERIFIER UNIMPLEMENTED: TODO(zyedidia)\n");
	}

RunnableScriptView 
LFIContract::to_view() const
{
    return RunnableScriptView(base->data(), base->size());
}

Hash
LFIContract::hash() const
{
    Hash out;
    hash_raw(base->data(), base->size(), out.data());
    return out;
}

} // namespace scs

