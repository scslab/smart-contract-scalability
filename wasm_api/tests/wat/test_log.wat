(module
  (import "scs" "host_log" (func $log (param i32) (param i32)))
  (import "scs" "get_calldata" (func $get_calldata (param i32 i32)))

  (memory 1)

  ;; call_log
  (func (export "pub00000000") (param $calldata_len i32) (result i32)

    (call $get_calldata (i32.const 0) (i32.const 8))

    ;; log 8 bytes
    i32.const 0
    i32.const 8
    call $log 

    ;; return 0
    i32.const 0)
)