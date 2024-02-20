#include "lficpp/lficpp.h"

namespace scs
{
	
LFIGlobalEngine::LFIGlobalEngine(lfi_syshandler handler)
	: lfi_engine(nullptr)
	{
		// how should I set these default options?
		struct lfi_options opts {
			.noverify = 1,
			.fastyield = 0,
			.pagesize = 0,
			.stacksize = 0, 
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
