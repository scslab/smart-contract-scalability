.macro syscall name num
.globl \name
\name:
	mov x8, \num
	svc #0
	ret
.endm

syscall _exit   500
syscall _write  501
syscall sbrk    502
syscall lseek   503
syscall read    504
syscall close   505

syscall lfihog_log 600
syscall lfihog_invoke 601
syscall lfihog_sender 602
syscall lfihog_self_addr 603
syscall lfihog_src_tx_hash 604
syscall lfihog_invoked_tx_hash 605


