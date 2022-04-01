(module
  (import "scs" "log" (func $log (param i32) (param i32)))
  (memory 1 1)

  ;; call_log
  (func (export "pub00000000") (param $calldata_len i32) (result i32)

    ;; log 8 bytes
    i32.const 0
    i32.const 8
    call $log 

    ;; return 0
    i32.const 0)
)