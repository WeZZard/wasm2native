#ifndef W2N_BASIC_UNIMPLEMENTED_H
#define W2N_BASIC_UNIMPLEMENTED_H

#include <llvm/Support/Debug.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <functional>
#include <w2n/Basic/Compiler.h>

#define w2n_assert(EXPR, ...)                                            \
  (__builtin_expect(!(EXPR), 0) ? w2n::details::report_assertion(        \
     __func__, __ASSERT_FILE_NAME, __LINE__, #EXPR, __VA_ARGS__          \
   )                                                                     \
                                : (void)0)

#define w2n_unimplemented()                                              \
  ::llvm::llvm_unreachable_internal(                                     \
    "not implemented.", __FILE__, __LINE__                               \
  )

/// Prototype implementation check-point function.
///
/// Supported Overloads:
/// ====================
/// - @code w2n_proto_implemented():
///
/// - @code w2n_proto_implemented(body):
///   @param body: A lambda expression which is the body of the prototype
///   implementation.
///
/// - @code w2n_proto_implemented(reason, body):
///   @param reason: A string literal which explains the reason why there
///   is a prototype implementation.
///   @param body: A lambda expression which is the body of the prototype
///   implementation.
///
#define w2n_proto_implemented(...)                                       \
  _W2N_PICK_MACRO_OVERLOAD_3(                                            \
    _0,                                                                  \
    ##__VA_ARGS__,                                                       \
    _w2n_proto_implemented_2,                                            \
    _w2n_proto_implemented_1,                                            \
    _w2n_proto_implemented_0                                             \
  )                                                                      \
  (__VA_ARGS__)

#define _w2n_proto_implemented_0()                                       \
  w2n::proto_implemented<void>(                                          \
    __FILE__, __LINE__, []() -> void {}, nullptr                         \
  )

#define _w2n_proto_implemented_1(BODY)                                   \
  w2n::proto_implemented<                                                \
    typename std::invoke_result<decltype(BODY)>::type>(                  \
    __FILE__, __LINE__, BODY, nullptr                                    \
  )

#define _w2n_proto_implemented_2(REASON, BODY)                           \
  w2n::proto_implemented<                                                \
    typename std::invoke_result<decltype(BODY)>::type>(                  \
    __FILE__, __LINE__, BODY, REASON                                     \
  )

namespace w2n {

namespace details {

void report_prototype_implementation(
  const char * File, unsigned Line, const char * Reason
);

W2N_NO_RETURN
void report_assertion(
  const char * Function,
  const char * File,
  int Line,
  const char * Expression,
  const char * Fmt,
  ...
) __cold __disable_tail_calls;

} // namespace details

// clang-format off
// Current version of clang-format cannot handle preceeding macros in
// seperated lines before a function declaration with
// `SeparateDefinitionBlocks` set to `Always`.

template <typename Result>
W2N_INLINE_ALWAYS
Result proto_implemented(
  const char * File,
  unsigned Line,
  std::function<Result(void)> Body,
  const char * Reason
) {
  details::report_prototype_implementation(File, Line, Reason);
  return Body();
}

// clang-format on

} // namespace w2n

#endif // W2N_BASIC_UNIMPLEMENTED_H
