#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <stdexcept>
<<<<<<< HEAD
#include <coroutine>
=======
>>>>>>> e379ce277881bdfd5a46ba5baa8b2cb171ec4de4


#include <utils/non_movable.h>

extern "C" {

#include "lfi.h"

}

namespace scs
{

class LFIGlobalEngine : public utils::NonMovableOrCopyable
{
	std::mutex mtx;

	struct lfi* lfi_engine;

public:
	LFIGlobalEngine(lfi_syshandler handler);

	~LFIGlobalEngine() {
		lfi_delete(lfi_engine);
	}

	int
	__attribute__((warn_unused_result))
	new_proc(struct lfi_proc** o_proc, struct lfi_proc_info* o_proc_info, std::vector<uint8_t> const& program_bytes, void* ctxp);

	void delete_proc(struct lfi_proc* proc);
};

<<<<<<< HEAD
struct LFIMessage {
	std::vector<uint8_t> bytes;
};

=======
>>>>>>> e379ce277881bdfd5a46ba5baa8b2cb171ec4de4
class LFIProc : public utils::NonMovableOrCopyable
{
	struct lfi_proc* proc;

	struct lfi_proc_info info;

	void* ctxp;

	LFIGlobalEngine& main_lfi;

public:

	LFIProc(void* ctxp, LFIGlobalEngine& main_lfi) 
		: proc(nullptr)
		, info()
		, ctxp(ctxp)
		, main_lfi(main_lfi)
		{}


	~LFIProc();

	int 
	__attribute__((warn_unused_result))
	set_program(std::vector<uint8_t> const& program_bytes);

<<<<<<< HEAD
	generator<LFIMessage> run(LFIMessage const& input);
=======
	void run();
>>>>>>> e379ce277881bdfd5a46ba5baa8b2cb171ec4de4
};

}
