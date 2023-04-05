;; RUN: %target-wat2wasm %s --output %t.wasm
;; RUN: %target-w2n-frontend %t.wasm -emit-ir | %FileCheck %s
(module
  (func $0 (local i32)
    i32.const 10
    local.set 0)
  (func $1 (result i32) (local i32)
    i32.const 10
    local.set 0
    local.get 0)
  ;; TODO: test case for "get before set is undefined behavior"
)

;; CHECK-LABEL: void @"function$0"()
;; CHECK: %"$local0" = alloca i32, align 4
;; CHECK: %"alloca point" = alloca i1, align 1
;; CHECK: %"local$0" = load i32, ptr %"$local0", align 4
;; CHECK: store i32 10, ptr %"$local0", align 4

;; CHECK-LABEL: i32 @"function$1"()
;; CHECK-LABEL: store i32 10, ptr %"$local0", align 4
;; CHECK-LABEL: store ptr %"$local0", ptr %"$return-value", align 4
;; CHECK-LABEL: %"$loaded-return-value" = load i32, ptr %"$return-value", align 4
;; CHECK-LABEL: ret i32 %"$loaded-return-value"
