#include "lficpp/lficpp.h"

#include <utility>

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

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
			.gas = 1'000'000,
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
LFIProc::set_program(const uint8_t* bytes, const size_t len)
{
	if (actively_running) {
		return -1;
	}
	if (len == 0) {
		if (bytes != NULL) {
			perror("invalid contract in db");
			std::abort();
		}
		return -1;
	}
	return lfi_proc_exec(proc, const_cast<uint8_t*>(bytes), len, &info);
}

uint32_t sandboxaddr(uintptr_t realaddr) {
	return realaddr & 0xFFFF'FFFF;
}

int
LFIProc::run(uint32_t method, std::vector<uint8_t> const& calldata)
{
	if (actively_running) {
		return -1;
	}
	lfi_proc_init_regs(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);

	brkbase = info.lastva;
	brksize = 0;

	struct lfi_regs* regs = lfi_proc_get_regs(proc);
	regs -> x0 = method;
	
	if (sbrk(calldata.size()) == static_cast<uintptr_t>(-1)) {
		return -1;
	}

	std::memcpy((void*)brkbase, calldata.data(), calldata.size());

	regs -> x1 = sandboxaddr(brkbase);
	regs -> x2 = calldata.size();

	actively_running = true;
	int code = lfi_proc_start(proc);

	if (brksize != 0)
	{
		if (munmap((void*) brkbase, brksize) != 0) {
			std::printf("cleanup failed\n");
			std::abort();
		}
	}

	actively_running = false;
	return code;
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


}
