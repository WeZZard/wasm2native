//===--- Compiler.h - Compiler specific definitions -------*- C++ -*-===//
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

#include <llvm/Support/Compiler.h>

#ifndef W2N_BASIC_COMPILER_H
#define W2N_BASIC_COMPILER_H

#if defined(_MSC_VER) && !defined(__clang__)
#define W2N_COMPILER_IS_MSVC 1
#else
#define W2N_COMPILER_IS_MSVC 0
#endif

// Workaround non-clang compilers
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

// __builtin_assume() is an optimization hint.
#if __has_builtin(__builtin_assume)
#define W2N_ASSUME(x) __builtin_assume(x)
#else
#define W2N_ASSUME(x)
#endif

/// Attributes.

#if __has_attribute(constructor)
#define W2N_CONSTRUCTOR __attribute__((constructor))
#else
#define W2N_CONSTRUCTOR
#endif

/// \macro W2N_GNUC_PREREQ
/// Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
#ifndef W2N_GNUC_PREREQ
#if defined(__GNUC__) && defined(__GNUC_MINOR__)                         \
  && defined(__GNUC_PATCHLEVEL__)
#define W2N_GNUC_PREREQ(maj, min, patch)                                 \
  ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__       \
   >= ((maj) << 20) + ((min) << 10) + (patch))
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define W2N_GNUC_PREREQ(maj, min, patch)                                 \
  ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10)                             \
   >= ((maj) << 20) + ((min) << 10))
#else
#define W2N_GNUC_PREREQ(maj, min, patch) 0
#endif
#endif

/// W2N_ATTRIBUTE_NOINLINE - On compilers where we have a directive to
/// do so, mark a method "not for inlining".
#if __has_attribute(noinline) || W2N_GNUC_PREREQ(3, 4, 0)
#define W2N_ATTRIBUTE_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define W2N_ATTRIBUTE_NOINLINE __declspec(noinline)
#else
#define W2N_ATTRIBUTE_NOINLINE
#endif

/// W2N_ATTRIBUTE_ALWAYS_INLINE - On compilers where we have a directive
/// to do so, mark a method "always inline" because it is performance
/// sensitive. GCC 3.4 supported this but is buggy in various cases and
/// produces unimplemented errors, just use it in GCC 4.0 and later.
#if __has_attribute(always_inline) || W2N_GNUC_PREREQ(4, 0, 0)
#define W2N_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define W2N_ATTRIBUTE_ALWAYS_INLINE __forceinline
#else
#define W2N_ATTRIBUTE_ALWAYS_INLINE
#endif

#ifdef __GNUC__
#define W2N_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define W2N_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define W2N_ATTRIBUTE_NORETURN
#endif

#ifndef W2N_BUG_REPORT_URL
#define W2N_BUG_REPORT_URL "https://github.com/WeZZard/wasm2native/issues"
#endif

#define W2N_BUG_REPORT_MESSAGE_BASE                                      \
  "submit a bug report (" W2N_BUG_REPORT_URL ")"

#define W2N_BUG_REPORT_MESSAGE "please " W2N_BUG_REPORT_MESSAGE_BASE

#define W2N_CRASH_BUG_REPORT_MESSAGE                                     \
  "Please " W2N_BUG_REPORT_MESSAGE_BASE                                  \
  " and include the crash backtrace."

// Conditionally exclude declarations or statements that are only needed
// for assertions from release builds (NDEBUG) without cluttering the
// surrounding code by #ifdefs.
//
// struct DoThings  {
//   W2N_ASSERT_ONLY_DECL(unsigned verifyCount = 0);
//   DoThings() {
//     W2N_ASSERT_ONLY(verifyCount = getNumberOfThingsToDo());
//   }
//   void doThings() {
//     do {
//       // ... do each thing
//       W2N_ASSERT_ONLY(--verifyCount);
//     } while (!done());
//     assert(verifyCount == 0 && "did not do everything");
//   }
// };
#ifdef NDEBUG
#define W2N_ASSERT_ONLY_DECL(...)
#define W2N_ASSERT_ONLY(...)                                             \
  do {                                                                   \
  } while (false)
#else
#define W2N_ASSERT_ONLY_DECL(...) __VA_ARGS__
#define W2N_ASSERT_ONLY(...)                                             \
  do {                                                                   \
    __VA_ARGS__;                                                         \
  } while (false)
#endif

#if defined(__LP64__) || defined(_WIN64)
#define W2N_POINTER_IS_8_BYTES 1
#define W2N_POINTER_IS_4_BYTES 0
#else
// TODO: consider supporting 16-bit targets
#define W2N_POINTER_IS_8_BYTES 0
#define W2N_POINTER_IS_4_BYTES 1
#endif

// Produce a string literal for the raw argument tokens.
#define W2N_STRINGIZE_RAW(TOK) #TOK

// Produce a string literal for the macro-expanded argument tokens.
#define W2N_STRINGIZE_EXPANDED(TOK) W2N_STRINGIZE_RAW(TOK)

#if defined(__USER_LABEL_PREFIX__)
#define W2N_SYMBOL_PREFIX_STRING                                         \
  W2N_STRINGIZE_EXPANDED(__USER_LABEL_PREFIX__)
#else
// Clang and GCC always define __USER_LABEL_PREFIX__, so this should
// only come up with MSVC, and Windows doesn't use a prefix.
#define W2N_SYMBOL_PREFIX_STRING ""
#endif

// An attribute to override the symbol name of a declaration.
// This does not compensate for platform symbol prefixes; for that,
// use W2N_ASM_LABEL_WITH_PREFIX.
//
// This only actually works on Clang or GCC; MSVC does not provide
// an attribute to change the asm label.
#define W2N_ASM_LABEL_RAW(STRING) __asm__(STRING)
#define W2N_ASM_LABEL_WITH_PREFIX(STRING)                                \
  W2N_ASM_LABEL_RAW(W2N_SYMBOL_PREFIX_STRING STRING)

// W2N_FORMAT(fmt,first) marks a function as taking a format string
// argument at argument `fmt`, with the first argument for the format
// string as `first`.
#if __has_attribute(format)
#define W2N_FORMAT(fmt, first) __attribute__((format(printf, fmt, first)))
#else
#define W2N_FORMAT(fmt, first)
#endif

// W2N_VFORMAT(fmt) marks a function as taking a format string argument
// at argument `fmt`, with the arguments in a `va_list`.
#if __has_attribute(format)
#define W2N_VFORMAT(fmt) __attribute__((format(printf, fmt, 0)))
#else
#define W2N_VFORMAT(fmt)
#endif

#if __has_attribute(enum_extensibility)
#define ENUM_EXTENSIBILITY_ATTR(arg)                                     \
  __attribute__((enum_extensibility(arg)))
#else
#define ENUM_EXTENSIBILITY_ATTR(arg)
#endif

#define W2N_NO_INLINE     [[noinline]]
#define W2N_INLINABLE     inline
#define W2N_INLINE_ALWAYS inline __attribute__((always_inline))
#define W2N_TRANSPARENT   inline __attribute__((always_inline))

#define W2N_NO_RETURN [[noreturn]]

#define W2N_UNUSED [[maybe_unused]]
#define W2N_USED   __attribute__((used))

#define W2N_DISABLE_TAIL_CALLS __attribute__((__disable_tail_calls__))
#define W2N_NOT_TAIL_CALLED    __attribute__((__not_tail_called__))

#define W2N_LIKELY_CALLED   __attribute__((__hot__))
#define W2N_UNLIKELY_CALLED __attribute__((__cold__))

#define _W2N_PICK_MACRO_OVERLOAD_1(_0, PICKED, ...)             PICKED
#define _W2N_PICK_MACRO_OVERLOAD_2(_0, _1, PICKED, ...)         PICKED
#define _W2N_PICK_MACRO_OVERLOAD_3(_0, _1, _2, PICKED, ...)     PICKED
#define _W2N_PICK_MACRO_OVERLOAD_4(_0, _1, _2, _3, PICKED, ...) PICKED

/// LLVM_NODISCARD - Warn if a type or return value is discarded.

// Use the 'nodiscard' attribute in C++17 or newer mode.
#if defined(__cplusplus) && __cplusplus > 201402L                        \
  && LLVM_HAS_CPP_ATTRIBUTE(nodiscard)
#define LLVM_NODISCARD [[nodiscard]]
#elif LLVM_HAS_CPP_ATTRIBUTE(clang::warn_unused_result)
#define LLVM_NODISCARD [[clang::warn_unused_result]]
// Clang in C++14 mode claims that it has the 'nodiscard' attribute, but
// also warns in the pedantic mode that 'nodiscard' is a C++17 extension
// (PR33518). Use the 'nodiscard' attribute in C++14 mode only with GCC.
// TODO: remove this workaround when PR33518 is resolved.
#elif defined(__GNUC__) && LLVM_HAS_CPP_ATTRIBUTE(nodiscard)
#define LLVM_NODISCARD [[nodiscard]]
#else
#define LLVM_NODISCARD
#endif

#endif // W2N_BASIC_COMPILER_H
