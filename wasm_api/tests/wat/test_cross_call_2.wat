(module
  (import "scs" "return" 
    (func $return 
      (param $return_offset i32) 
      (param $return_len i32) 
    )
  )

  (import "scs" "log" (func $log (param i32) (param i32)))

  (memory 1 1)

  ;; input is address of target to call
  (func (export "FF000000") (result i32)

    (i32.store (i32.const 255) (i32.const 0xFF))

    ;; log 4 bytes
    i32.const 255
    i32.const 4

    call $log

    ;; return 55
    i32.const 55
  )
)