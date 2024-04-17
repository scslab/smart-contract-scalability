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
syscall lfihog_block_number 606

syscall lfihog_has_key 607

syscall lfihog_raw_mem_set 608
syscall lfihog_raw_mem_get 609
syscall lfihog_raw_mem_get_len 610

syscall lfihog_delete_key_last 611

syscall lfihog_nnint_set_add 612
syscall lfihog_nnint_add 613
syscall lfihog_nnint_get 614

syscall lfihog_hs_insert 615
syscall lfihog_hs_inc_limit 616
syscall lfihog_hs_clear 617
syscall lfihog_hs_get_size 618
syscall lfihog_hs_get_max_size 619

syscall lfihog_hs_get_index_of 620
syscall lfihog_hs_get_index 621

syscall lfihog_contract_create 622
syscall lfihog_contract_deploy 623

syscall lfihog_witness_get 624
syscall lfihog_witness_get_len 625

