#ifndef W2N_BASIC_UNIMPLEMENTED_H
#define W2N_BASIC_UNIMPLEMENTED_H

#include <llvm/Support/ErrorHandling.h>
#include <functional>
#include <w2n/Basic/Compiler.h>

#define w2n_not_implemented()                                            \
  ::llvm::llvm_unreachable_internal(                                     \
    "not implemented.", __FILE__, __LINE__                               \
  )

namespace w2n {

// Current version of clang-format cannot handle preceeding macros in
// seperated lines before a function declaration with
// `SeparateDefinitionBlocks` set to `Always`.

// clang-format off
template <typename Result>
W2N_INLINE_ALWAYS
Result proto_impl(std::function<Result(void)> body) {
  return body();
}

template <>
W2N_INLINE_ALWAYS
void proto_impl<void>(std::function<void(void)> body) {
  return body();
}

W2N_INLINE_ALWAYS
void proto_impl() {
  
}

// clang-format on

} // namespace w2n

#endif // W2N_BASIC_UNIMPLEMENTED_H
