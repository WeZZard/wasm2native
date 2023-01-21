#ifndef W2N_AST_TABLE_H
#define W2N_AST_TABLE_H

#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>

namespace w2n {

/**
 * @brief Represents a table in WebAssembly.
 *
 * Wasm file guarantees one-pass validation. This makes informations about
 * one object in wasm file are speratedly located in the file. This class
 * is designed to coalesce the separated info about tables into one place.
 */
class Table : public llvm::ilist_node<Table>, public ASTAllocated<Table> {
public:

  ~Table(){};
};

} // namespace w2n

//===----------------------------------------------------------------------===//
// ilist_traits for Table
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::w2n::Table> :
  public ilist_node_traits<::w2n::Table> {
  using Table = ::w2n::Table;

public:

  static void deleteNode(Table * V) {
    V->~Table();
  }

private:

  void createNode(const Table&);
};

} // namespace llvm

#endif // W2N_AST_TABLE_H
