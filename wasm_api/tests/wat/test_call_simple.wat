(module
  ;; "add" for direct wasm3 api test
  (func (export "add") (param $lhs i32) (param $rhs i32) (result i32)
    local.get $lhs
    local.get $rhs
    i32.add)

  (func (export "01000000") (result i32)
    i32.const 0)
)