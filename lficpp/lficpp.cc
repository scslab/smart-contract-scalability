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
	
LFIGlobalEngine::LFIGlobalEngine(lfi_syshandler handler)
	: lfi_engine(nullptr)
	{
		struct lfi_options opts {
			.noverify = 1, // programs should be verified when they're registered, not at runtime
			.fastyield = 0, // this option does nothing at the moment TODO
			.pagesize = (size_t) getpagesize(),
			.stacksize = 65536, 
			.syshandler = handler,
			.gas = 0,			
			.poc = 1
		};

		lfi_engine = lfi_new(opts);

		std::printf("add vaspaces\n");
        lfi_auto_add_vaspaces(lfi_engine,  8ULL * 1024  * 1024 * 1024 * 1024);
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


int 
LFIGlobalEngine::new_proc(struct lfi_proc** o_proc, void* ctxp)
{
	std::lock_guard lock(mtx);
	active_proc_count++;
	return lfi_add_proc(lfi_engine, o_proc, ctxp);
}

void 
LFIGlobalEngine::delete_proc(struct lfi_proc* proc)
{
	std::lock_guard lock(mtx);
	lfi_remove_proc(lfi_engine, proc);
	active_proc_count--;
}

LFIProc::LFIProc(void* ctxp, LFIGlobalEngine& main_lfi) 
	: proc(nullptr)
	, info()
	, ctxp(ctxp)
	, main_lfi(main_lfi)
{
	if (main_lfi.new_proc(&proc, ctxp) != 0)
	{
		proc = nullptr;
	} else {
        base = lfi_proc_base(proc);
    }
}

uint64_t
LFIProc::addr(uint64_t a) {
    return base | ((uint32_t) a);
}

int 
LFIProc::set_program(RunnableScriptView const& script)
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
	return lfi_proc_exec(proc, const_cast<uint8_t*>(script.data), script.len, &info);
}

uint32_t sandboxaddr(uintptr_t realaddr) {
	return realaddr & 0xFFFF'FFFF;
}

uint64_t
LFIProc::run(uint32_t method, std::vector<uint8_t> const& calldata, uint32_t gas)
{
	if (actively_running) {
		throw HostError("reentrance in LFIProc");
	}
	lfi_proc_init_regs(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);

	brkbase = info.lastva;
	brksize = 0;

	struct lfi_regs* regs = lfi_proc_get_regs(proc);
	regs -> x0 = method;
	
	if (sbrk(calldata.size()) == static_cast<uintptr_t>(-1)) {
		throw HostError("failed to allocate calldata");
	}

	std::memcpy((void*)brkbase, calldata.data(), calldata.size());

	regs -> x1 = sandboxaddr(brkbase);
	regs -> x2 = calldata.size();

	regs -> x23 = sandboxaddr(gas);

	std::printf("gas was %u set is %u\n", gas, regs -> x23);

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
	std::printf("remaining gas is %lu\n", remaining_gas);
	return gas - remaining_gas;
}

void
LFIProc::deduct_gas(uint64_t gas) {
	struct lfi_regs* regs = lfi_proc_get_regs(proc);
	if (gas > regs -> x23) {
		throw HostError("gas usage exceeded limit in deduct_gas");
	}
	regs -> x23 -= gas;
}
uint64_t 
LFIProc::get_available_gas() {
	struct lfi_regs* regs = lfi_proc_get_regs(proc);
	return regs -> x23;
}

void 
LFIProc::exit(int code) {
	if (!actively_running) {
		std::printf("bizarre\n");
		std::abort();
	}
	actively_running = false;
	lfi_proc_exit(proc, code);
	std::unreachable();
}

LFIProc::~LFIProc()
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
LFIProc::sbrk(uint32_t incr)
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
LFIProc::is_writable(uintptr_t p, uint32_t size) const
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
LFIProc::is_readable(uint64_t p, uint32_t size) const {
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

