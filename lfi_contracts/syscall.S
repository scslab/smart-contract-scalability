.macro syscall name num
.globl \name
\name:
	mov x8, \num
	svc #0
	ret
.endm

syscall exit   500
syscall write  501
