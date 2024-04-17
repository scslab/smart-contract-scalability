#include "lficpp/signal_handler.h"

#include <stdexcept>
#include <cstdint>
#include <signal.h>

namespace scs
{

#if defined(__aarch64__)

static void signal_trap_handler(int sig, siginfo_t* si, void* context) {
    // TODO: check if the signal arrived while a LFI process was executing, by
    // checking if the PC is inside a sandbox. Currently we assume this is the
    // case, meaning that if Groundhog itself causes a SIGSEGV, this will
    // behave badly.
    ucontext_t* uctx = (ucontext_t*) context;
    uint64_t saved = lfi_signal_start(uctx->uc_mcontext.regs[21]);
    struct lfi_proc* proc = lfi_sys_proc(uctx->uc_mcontext.regs[21]);
    std::printf("received signal, pc: %llx\n", uctx->uc_mcontext.pc);

    lfi_proc_exit(proc, sig);

    // unreachable
    std::abort();
    lfi_signal_end(saved);
}

static void signal_register(int sig) {
    struct sigaction act;
    act.sa_sigaction = &signal_trap_handler;
    act.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    if (sigaction(sig, &act, NULL) != 0) {
        perror("sigaction");
        std::abort();
    }
}


static thread_local bool initialized = false;

void signal_init() {
	if (initialized) return;

    stack_t ss;
    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == NULL) {
        perror("malloc");
        std::abort();
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1) {
        perror("sigaltstack");
        std::abort();
    }

    signal_register(SIGSEGV);
    signal_register(SIGILL);
    signal_register(SIGTRAP);
    signal_register(SIGFPE);
    signal_register(SIGBUS);
    initialized = true;
}

#else

void signal_init() {
	throw std::runtime_error("wrong architecture");
}

#endif

}