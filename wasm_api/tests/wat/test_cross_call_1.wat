(module
  (import "scs" "invoke" 
    (func $invoke 
      (param $addr_offset i32) 
      (param $methodname i32) 
      (param $calldata_offset i32)
      (param $calldata_len i32)
      (result i32)
    )
  )

  (import "scs" "get_calldata" (func $get_calldata (param i32 i32)))

  (import "scs" "host_log" (func $log (param i32) (param i32)))

  (memory 1 1)

  ;; input is address of target to call
  (func (export "00000000") (result i32)

    (local $callres i32)

    ;; get_calldata
    i32.const 0
    i32.const 32

    call $get_calldata

    ;; addr_offset
    i32.const 0

    ;; methodname
    i32.const 0x000000FF

    ;; calldata
    i32.const 0
    i32.const 0

    call $invoke 

    local.set $callres

    (i32.store (i32.const 0) (local.get $callres))

    ;; log 4 bytes
    i32.const 0
    i32.const 4

    call $log

    ;; return 0
    i32.const 0
  )
)