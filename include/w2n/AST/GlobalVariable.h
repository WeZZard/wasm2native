#ifndef W2N_AST_GLOBALVARIABLE_H
#define W2N_AST_GLOBALVARIABLE_H

#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Type.h>

namespace w2n {

class Function;

/**
 * @brief Represents a global variable in WebAssembly.
 *
 * Wasm file guarantees one-pass validation. This makes informations about
 * one object in wasm file are speratedly located in the file. This class
 * is designed to coalesce the separated info about global variables into
 * one place.
 */
class GlobalVariable :
  public llvm::ilist_node<GlobalVariable>,
  public ASTAllocated<GlobalVariable> {
private:

  ModuleDecl& Module;
  ASTLinkage Linkage;
  uint32_t Index;
  llvm::Optional<Identifier> Name;
  ValueType * Ty;
  bool IsMutable;
  // Function& Init;
  GlobalDecl * Decl;

  GlobalVariable(
    ModuleDecl& Module,
    ASTLinkage Linkage,
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    ValueType * Ty,
    bool IsMutable,
    // Function& Init,
    GlobalDecl * Decl
  );

public:

  static GlobalVariable * create(
    ModuleDecl& Module,
    ASTLinkage Linkage,
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    ValueType * Ty,
    bool IsMutable,
    // Function& Init,
    GlobalDecl * Decl = nullptr
  );

  ~GlobalVariable() {
  }

  ModuleDecl& getModule() {
    return Module;
  };

  const ModuleDecl& getModule() const {
    return Module;
  }

  ASTLinkage getASTLinkage() const {
    return Linkage;
  }

  uint32_t getIndex() const {
    return Index;
  }

  llvm::Optional<Identifier> getName() const {
    return Name;
  }

  ValueType * getType() {
    return Ty;
  }

  ValueType * getType() const {
    return Ty;
  }

  bool isMutable() const {
    return IsMutable;
  }

  GlobalDecl * getDecl() {
    return Decl;
  };

  const GlobalDecl * getDecl() const {
    return Decl;
  }

  /// A name used for debugging the compiler like: global$0, global$1 ...
  std::string getDescriptiveName() const;

  /// A full qualified name used for debugging the compiler like:
  /// module.global$0, module.global$1 ...
  std::string getFullQualifiedDescriptiveName() const;
};

} // namespace w2n

//===----------------------------------------------------------------------===//
// ilist_traits for GlobalVariable
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::w2n::GlobalVariable> :
  public ilist_node_traits<::w2n::GlobalVariable> {
  using GlobalVariable = ::w2n::GlobalVariable;

public:

  static void deleteNode(GlobalVariable * V) {
    V->~GlobalVariable();
  }

private:

  void createNode(const GlobalVariable&);
};

} // namespace llvm

#endif // W2N_AST_GLOBALVARIABLE_H
