/// This file defines common diagnostics for the whole compiler, as well
/// as some diagnostic infrastructure.

#ifndef W2N_DIAGNOSTICSCOMMON_H
#define W2N_DIAGNOSTICSCOMMON_H

#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {
template <typename... ArgTypes>
struct Diag;

namespace detail {
// These templates are used to help extract the type arguments of the
// DIAG/ERROR/WARNING/NOTE/REMARK/FIXIT macros.
template <typename T>
struct DiagWithArguments;

template <typename... ArgTypes>
struct DiagWithArguments<void(ArgTypes...)> {
  typedef Diag<ArgTypes...> type;
};

template <typename T>
struct StructuredFixItWithArguments;

template <typename... ArgTypes>
struct StructuredFixItWithArguments<void(ArgTypes...)> {
  typedef StructuredFixIt<ArgTypes...> type;
};
} // end namespace detail

enum class StaticSpellingKind : uint8_t;

namespace diag {

enum class RequirementKind : uint8_t;

// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND, ID, Options, Text, Signature)                         \
  extern detail::DiagWithArguments<void Signature>::type ID;
#define FIXIT(ID, Text, Signature)                                       \
  extern detail::StructuredFixItWithArguments<void Signature>::type ID;
#include <w2n/AST/DiagnosticsCommon.def>
} // end namespace diag
} // end namespace w2n

#endif
