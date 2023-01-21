#ifndef W2N_AST_MEMORY_H
#define W2N_AST_MEMORY_H

#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>

namespace w2n {

/**
 * @brief Represents a memory in WebAssembly.
 *
 * Wasm file guarantees one-pass validation. This makes informations about
 * one object in wasm file are speratedly located in the file. This class
 * is designed to coalesce the separated info about memories into one
 * place.
 */
class Memory :
  public llvm::ilist_node<Memory>,
  public ASTAllocated<Memory> {
public:

  ~Memory() {
  }
};

} // namespace w2n

//===----------------------------------------------------------------------===//
// ilist_traits for Memory
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::w2n::Memory> :
  public ilist_node_traits<::w2n::Memory> {
  using Memory = ::w2n::Memory;

public:

  static void deleteNode(Memory * V) {
    V->~Memory();
  }

private:

  void createNode(const Memory&);
};

} // namespace llvm

#endif // W2N_AST_MEMORY_H
