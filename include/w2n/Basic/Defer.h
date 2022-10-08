/// This file defines a 'W2N_DEFER' macro for performing a cleanup on any
/// exit out of a scope.

#ifndef W2N_BASIC_DEFER_H
#define W2N_BASIC_DEFER_H

#include <llvm/ADT/ScopeExit.h>

namespace w2n {
namespace detail {
struct DeferTask {};

template <typename F>
auto operator+(DeferTask, F&& fn)
  -> decltype(llvm::make_scope_exit(std::forward<F>(fn))) {
  return llvm::make_scope_exit(std::forward<F>(fn));
}
} // namespace detail
} // end namespace w2n

#define DEFER_CONCAT_IMPL(x, y)  x##y
#define DEFER_MACRO_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)

/// This macro is used to register a function / lambda to be run on exit
/// from a scope.  Its typical use looks like:
///
///   W2N_DEFER {
///     stuff
///   };
///
#define W2N_DEFER                                                        \
  auto DEFER_MACRO_CONCAT(defer_func, __COUNTER__) =                     \
    ::w2n::detail::DeferTask() + [&]()

#endif // W2N_BASIC_DEFER_H
