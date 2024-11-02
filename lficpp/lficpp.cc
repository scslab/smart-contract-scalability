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
            .sysexternal = 0,
            .pagesize = 4 * 1024,
            .stacksize = 65536, 
            .gas = 1'000'000,
            .syshandler = handler,
        };

        lfi_engine = lfi_new(opts);
        assert(lfi_engine);

        std::printf("add vaspaces\n");
        bool ok = lfi_reserve(lfi_engine,  8ULL * 1024  * 1024 * 1024 * 1024); // 8TB
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

enum {
    KB = 1024,
};

bool 
LFIGlobalEngine::new_proc(LFIProc** o_proc, void* ctxp)
{
    std::lock_guard lock(mtx);
    active_proc_count++;
    return lfi_adduproc(lfi_engine, o_proc, ctxp, 0x20000, 128 * KB, 128 * KB);
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
    brkfull = info.extradata;

    LFIRegs* regs = lfi_proc_regs(proc);
    *lfi_regs_arg(regs, 0) = method;
    
    if (sbrk(calldata.size()) == static_cast<uintptr_t>(-1)) {
        throw HostError("failed to allocate calldata");
    }

    std::memcpy((void*)brkbase, calldata.data(), calldata.size());

    *lfi_regs_arg(regs, 1) = sandboxaddr(brkbase);
    *lfi_regs_arg(regs, 2) = calldata.size();

    uint64_t* r;
    if ((r = lfi_regs_gas(regs)))
        *r = gas;
    // else
    //     fprintf(stderr, "warning: LFI does not support gas tracking on this architecture\n");

    actively_running = true;
    int code = lfi_proc_start(proc);
    actively_running = false;

    if (code != 0) {
        throw HostError("lfi_proc_start returned errorcode");
    }

    if (brksize != 0 && brksize > brkfull)
    {
        if (munmap((void*) brkbase, brksize) != 0) {
            std::printf("cleanup failed\n");
            std::abort();
        }
    }

    uint64_t remaining_gas;
    if (r) {
        remaining_gas = *r;
    } else {
        remaining_gas = gas;
    }
    if (remaining_gas > gas) {
        throw HostError("gas usage exceeded limit");
    }
    return gas - remaining_gas;
}

void
DeClProc::deduct_gas(uint64_t gas) {
    LFIRegs* regs = lfi_proc_regs(proc);
    uint64_t* r;
    if ((r = lfi_regs_gas(regs))) {
        if (gas > *r) {
            throw HostError("gas usage exceeded limit in deduct_gas");
        }
        *r -= gas;
    }
}

uint64_t 
DeClProc::get_available_gas() {
    LFIRegs* regs = lfi_proc_regs(proc);
    uint64_t* r;
    if ((r = lfi_regs_gas(regs))) {
        return *r;
    }
    return UINT64_MAX;
}

void 
DeClProc::exit(int code) {
    if (!actively_running) {
        std::printf("bizarre\n");
        std::abort();
    }
    actively_running = false;
    lfi_proc_exit(code);
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
        if (brksize == 0 && brkfull == 0) {
            // map = mmap((void*) brkbase, brksize + incr, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
            fprintf(stderr, "excessive allocation would require remapping\n");
            std::abort();
        } else if (brksize + incr > brkfull) {
            // map = mremap((void*) brkbase, brksize, brksize + incr, 0);
            fprintf(stderr, "excessive allocation would require remapping\n");
            std::abort();
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
    // TODO: provide API for this in liblfi
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
