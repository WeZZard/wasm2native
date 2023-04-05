;; RUN: %target-wat2wasm %s --output %t.wasm
;; RUN: %target-w2n-frontend %t.wasm -emit-ir | %FileCheck %s
(module
  (func $f0)
  (func $f2 (local i32))
  (func $f3 (param i32))
  (func $f4 (param i32) (result i32)
    i32.const 10)
)

;; CHECK-LABEL: void @"function$0"()
;; CHECK: %"alloca point" = alloca i1, align 1

;; CHECK-LABEL: void @"function$1"()
;; CHECK: %"$local0" = alloca i32, align 4
;; CHECK: %"alloca point" = alloca i1, align 1

;; CHECK-LABEL: void @"function$2"(i32 %0)
;; CHECK: %"$local0 aka $arg0" = alloca i32, align 4
;; CHECK: %"alloca point" = alloca i1, align 1
;; CHECK: store i32 %0, ptr %"$local0 aka $arg0", align 4

;; CHECK-LABEL: i32 @"function$3"(i32 %0)
;; CHECK: %"$local0 aka $arg0" = alloca i32, align 4
;; CHECK: %"alloca point" = alloca i1, align 1
;; CHECK: store i32 %0, ptr %"$local0 aka $arg0", align 4
;; CHECK: store i32 10, ptr %"$return-value", align 4
;; CHECK: %"$loaded-return-value" = load i32, ptr %"$return-value", align 4
;; CHECK: ret i32 %"$loaded-return-value"
