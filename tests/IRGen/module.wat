;; RUN: %target-wat2wasm %s --output %t.wasm
;; RUN: %target-w2n-frontend %t.wasm -emit-ir | %FileCheck %s
;; CHECK: ; ModuleID
(module
  
)
