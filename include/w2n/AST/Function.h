#ifndef W2N_AST_FUNCTION
#define W2N_AST_FUNCTION

#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Type.h>

namespace w2n {

/**
 * @brief Represents a function in WebAssembly.
 *
 * Wasm file guarantees one-pass validation. This makes informations about
 * one object in wasm file are speratedly located in the file. This class
 * is designed to coalesce the separated info about functions into one
 * place.
 */
class Function :
  public llvm::ilist_node<Function>,
  public ASTAllocated<Function> {};

} // namespace w2n

//===----------------------------------------------------------------------===//
// ilist_traits for Function
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::w2n::Function> :
  public ilist_node_traits<::w2n::Function> {
  using Function = ::w2n::Function;

public:

  static void deleteNode(Function * V) {
    V->~Function();
  }

private:

  void createNode(const Function&);
};

} // namespace llvm

#endif // W2N_AST_FUNCTION