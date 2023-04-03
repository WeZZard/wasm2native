;; RUN: %target-wat2wasm %s --output %t.wasm
;; RUN: %target-w2n-frontend %t.wasm -emit-ir | %FileCheck %s
(module
  ;; CHECK: @".global$0" = internal global i32 0, align 4
  (global $a (mut i32) (i32.const 0))
  ;; CHECK: @".global$1" = internal global i32 0, align 4
  (global $b (mut i32) (i32.const 0))
)
