.macro defsys name num
.p2align 4
.globl \name
\name:
#if defined(__aarch64__)
	mov x8, \num
	svc #0
	ret
#elif defined(__x86_64__)
    mov $\num, %rax
    mov %rcx, %r10
    syscall
    ret
#endif
.endm

defsys _exit   500
defsys _write  501

defsys sbrk    503
defsys lseek   504
defsys read    505
defsys close   506

defsys lfihog_log 600
defsys lfihog_invoke 601
defsys lfihog_sender 602
defsys lfihog_self_addr 603
defsys lfihog_src_tx_hash 604
defsys lfihog_invoked_tx_hash 605
defsys lfihog_block_number 606

defsys lfihog_has_key 607

defsys lfihog_raw_mem_set 608
defsys lfihog_raw_mem_get 609
defsys lfihog_raw_mem_get_len 610

defsys lfihog_delete_key_last 611

defsys lfihog_nnint_set_add 612
defsys lfihog_nnint_add 613
defsys lfihog_nnint_get 614

defsys lfihog_hs_insert 615
defsys lfihog_hs_inc_limit 616
defsys lfihog_hs_clear 617
defsys lfihog_hs_get_size 618
defsys lfihog_hs_get_max_size 619

defsys lfihog_hs_get_index_of 620
defsys lfihog_hs_get_index 621

defsys lfihog_contract_create 622
defsys lfihog_contract_deploy 623

defsys lfihog_witness_get 624
defsys lfihog_witness_get_len 625

defsys lfihog_return 626

defsys lfihog_gas 629
