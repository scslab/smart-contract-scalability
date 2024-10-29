#include "lficpp/signal_handler.h"

#include <stdexcept>
#include <cstdint>
#include <signal.h>


extern "C" {

#include "lfi.h"

}


namespace scs
{

static void signal_trap_handler(int sig, siginfo_t* si, void* context) {
    (void) si;

    ucontext_t* uctx = (ucontext_t*) context;

    uintptr_t pc;
#if defined(__aarch64__)
    pc = uctx->uc_mcontext.pc;
#elif defined(__x86_64__)
    pc = uctx->uc_mcontext.gregs[REG_RIP];
#endif

    std::printf("received signal %d, pc: %lx\n", sig, pc);

    if (lfi_proc())
        lfi_proc_exit(sig);

    std::printf("no active LFI process\n");
    // unreachable
    std::abort();
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

}
