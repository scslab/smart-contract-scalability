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


LFIProc 
LFIGlobalEngine::new_proc(std::vector<uint8_t> const& program_bytes, void* ctxp)
{
	std::lock_guard lock(mtx);

	int err = 0;
	struct lfi_proc_info info;

	auto* proc = lfi_add_proc(lfi_engine, const_cast<uint8_t*>(program_bytes.data()), program_bytes.size(), ctxp, &info, &err);

	if (err != 0) {
		throw std::runtime_error("failed to start lfi proc");
	}
	return LFIProc(proc, info, *this);
}

void 
LFIGlobalEngine::delete_proc(LFIProc& proc)
{
	std::lock_guard lock(mtx);
	lfi_remove_proc(lfi_engine, proc.proc);
}

void 
LFIProc::reset_program(std::vector<uint8_t> const& program_bytes)
{
	int err = lfi_proc_exec(proc, const_cast<uint8_t*>(program_bytes.data()), program_bytes.size(), &info);

	if (err != 0) {
		throw std::runtime_error("invalid lfi program change");
	}
}

void
LFIProc::run()
{
	lfi_proc_init_regs(proc, info.elfentry, reinterpret_cast<uintptr_t>(info.stack) + info.stacksize - 16);
	lfi_proc_start(proc);
}


}
