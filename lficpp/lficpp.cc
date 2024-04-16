#include "lficpp/lficpp.h"

#include <utility>

#include <unistd.h>
#include <sys/mman.h>

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
			.syshandler = handler
		};

		lfi_engine = lfi_new(opts);

	std::printf("add vaspaces\n");
        lfi_auto_add_vaspaces(lfi_engine);
	}


int 
LFIGlobalEngine::new_proc(struct lfi_proc** o_proc, void* ctxp)
{
	std::lock_guard lock(mtx);

	return lfi_add_proc(lfi_engine, o_proc, ctxp);
}

void 
LFIGlobalEngine::delete_proc(struct lfi_proc* proc)
{
	std::lock_guard lock(mtx);
	lfi_remove_proc(lfi_engine, proc);
}

LFIProc::LFIProc(void* ctxp, LFIGlobalEngine& main_lfi) 
	: proc(nullptr)
	, info()
	, ctxp(ctxp)
	, main_lfi(main_lfi)
{
	if (main_lfi.new_proc(&proc, ctxp) != 0)
	{
		std::printf("wtf\n");
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
	return lfi_proc_exec(proc, const_cast<uint8_t*>(bytes), len, &info);
}

int
LFIProc::run()
{
	if (actively_running) {
		return -1;
	}
	actively_running = true;
	lfi_proc_init_regs(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);

    brkbase = info.lastva;
    brksize = 0;

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

}
