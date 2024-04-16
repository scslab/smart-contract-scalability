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

