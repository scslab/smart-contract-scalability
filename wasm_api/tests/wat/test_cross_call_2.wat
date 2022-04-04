(module
  (import "scs" "return" 
    (func $return 
      (param $return_offset i32) 
      (param $return_len i32) 
    )
  )

  (import "scs" "host_log" (func $log (param i32) (param i32)))

  (memory 1)

  ;; input is address of target to call
  (func (export "pubFF000000") (param $calldata_len i32)

    (i32.store (i32.const 255) (i32.const 0xFF))

    ;; log 4 bytes
    i32.const 255
    i32.const 4

    call $log
  )
)