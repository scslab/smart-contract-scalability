#include "lficpp/lficpp.h"

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
LFIGlobalEngine::new_proc(struct lfi_proc** o_proc, struct lfi_proc_info* o_proc_info, std::vector<uint8_t> const& program_bytes, void* ctxp)
{
	std::lock_guard lock(mtx);

	int err = 0;

	(*o_proc) = lfi_add_proc(lfi_engine, const_cast<uint8_t*>(program_bytes.data()), program_bytes.size(), ctxp, o_proc_info, &err);

	return err;
}

void 
LFIGlobalEngine::delete_proc(struct lfi_proc* proc)
{
	std::lock_guard lock(mtx);
	lfi_remove_proc(lfi_engine, proc);
}

int 
LFIProc::set_program(std::vector<uint8_t> const& program_bytes)
{
	if (proc == nullptr) {
		return main_lfi.new_proc(&proc, &info, program_bytes, ctxp);
	}

	return lfi_proc_exec(proc, const_cast<uint8_t*>(program_bytes.data()), program_bytes.size(), &info);
}

void
LFIProc::run()
{
	lfi_proc_init_regs(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);
	lfi_proc_start(proc);
}

LFIProc::~LFIProc()
{
	if (proc != nullptr)
	{
		main_lfi.delete_proc(proc);
	}
}

generator<LFIMessage> 
LFIProc::run(LFIMessage const& input)
{
	
}



}
