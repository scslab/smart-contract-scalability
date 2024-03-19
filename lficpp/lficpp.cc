#include "lficpp/lficpp.h"

#include <utility>

#include <unistd.h>

namespace scs
{
	
LFIGlobalEngine::LFIGlobalEngine(lfi_syshandler handler)
	: lfi_engine(nullptr)
	{
		struct lfi_options opts {
			.noverify = 1, // programs should be verified when they're registered, not at runtime
			.fastyield = 0, // this option does nothing at the moment TODO
			.pagesize = std::max<size_t>(getpagesize(), SIZE_MAX),
			.stacksize = 65536, 
			.syshandler = handler
		};

		lfi_engine = lfi_new(opts);
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
		proc = nullptr;
	}
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
	int code = lfi_proc_start(proc);
	actively_running = false;
	return code;
}

void 
LFIProc::exit_error() {
	if (!actively_running) {
		std::printf("bizarre\n");
		std::abort();
	}
	actively_running = false;
	lfi_proc_exit(proc, 1);
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