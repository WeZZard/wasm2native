//===-------- OptimizationMode.h - Optimization modes -----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//

#ifndef W2N_BASIC_OPTIMIZATIONMODE_H
#define W2N_BASIC_OPTIMIZATIONMODE_H

#include <llvm/Support/DataTypes.h>

namespace w2n {

// The optimization mode specified on the command line or with function
// attributes.
enum class OptimizationMode : uint8_t {
  NotSet = 0,
  NoOptimization = 1, // -Onone
  ForSpeed = 2,       // -Ospeed == -O
  ForSize = 3,        // -Osize
  LastMode = ForSize
};

} // namespace w2n

#endif // W2N_BASIC_OPTIMIZATIONMODE_H
