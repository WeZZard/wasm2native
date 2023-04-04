;; RUN: %target-wat2wasm %s --output %t.wasm
;; RUN: %target-w2n-frontend %t.wasm -emit-ir | %FileCheck %s
(module
  (global $a (mut i32) (i32.const 0))
  (global $b (mut i32) (i32.const 10))
)
;; CHECK: @".global$0" = internal global i32 0, align 4
;; CHECK: @".global$1" = internal global i32 0, align 4

;; CHECK-LABEL: @"global-init$0"(
;; CHECK: %"$return-value" = alloca i32, align 4
;; CHECK: store i32 0, ptr %"$return-value", align 4
;; CHECK: %"$loaded-return-value" = load i32, ptr %"$return-value", align 4
;; CHECK: ret i32 %"$loaded-return-value"

;; CHECK-LABEL: @constructor
;; CHECK: %0 = call i32 @"global-init$0"()
;; CHECK: store i32 %0, ptr @".global$0", align 4
;; CHECK: ret void

;; CHECK-LABEL: @"global-init$1"(
;; CHECK: %"$return-value" = alloca i32, align 4
;; CHECK: store i32 10, ptr %"$return-value", align 4
;; CHECK: %"$loaded-return-value" = load i32, ptr %"$return-value", align 4
;; CHECK: ret i32 %"$loaded-return-value"

;; CHECK-LABEL: @constructor.1
;; CHECK: %0 = call i32 @"global-init$1"()
;; CHECK: store i32 %0, ptr @".global$1", align 4
;; CHECK: ret void
