#ifndef W2N_AST_FUNCTION_H
#define W2N_AST_FUNCTION_H

#include <_types/_uint32_t.h>
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Type.h>

namespace w2n {

/**
 * @brief Represents a function or the init procedure of a global in
 * WebAssembly.
 *
 * Wasm file guarantees one-pass validation. This makes informations about
 * one object in wasm file are speratedly located in the file. This class
 * is designed to coalesce the separated info about functions into one
 * place.
 */
class Function :
  public llvm::ilist_node<Function>,
  public ASTAllocated<Function> {
public:

  enum class FunctionKind {
    GlobalInit,
    Function,
  };

private:

  FunctionKind Kind;

  uint32_t Index;

  llvm::Optional<Identifier> Name;

  FuncTypeDecl * Type;

  /// @note: Global variable's init expression does not have locals.
  std::vector<LocalDecl *> Locals;

  ExpressionDecl * Expression;

  bool Exported;

  Function(
    FunctionKind Kind,
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) :
    Kind(Kind),
    Index(Index),
    Name(Name),
    Type(Type),
    Locals(Locals),
    Expression(Expression),
    Exported(IsExported) {
  }

public:

  ~Function() {
  }

  static Function * createFunction(
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) {
    return new (Expression->getASTContext()) Function(
      FunctionKind::Function,
      Index,
      Name,
      Type,
      Locals,
      Expression,
      IsExported
    );
  }

  static Function * createInit(
    uint32_t Index,
    FuncTypeDecl * Type,
    ExpressionDecl * Expression,
    llvm::Optional<Identifier> Name
  ) {
    return new (Expression->getASTContext()) Function(
      FunctionKind::GlobalInit, Index, Name, Type, {}, Expression, false
    );
  }

  FunctionKind getKind() const {
    return Kind;
  }

  bool isGlobalInit() const {
    return Kind == FunctionKind::GlobalInit;
  }

  uint32_t getIndex() const {
    return Index;
  }

  FuncTypeDecl * getType() {
    return Type;
  }

  const FuncTypeDecl * getType() const {
    return Type;
  }

  std::vector<LocalDecl *>& getLocals() {
    return Locals;
  }

  const std::vector<LocalDecl *>& getLocals() const {
    return Locals;
  }

  ExpressionDecl * getExpression() {
    return Expression;
  }

  const ExpressionDecl * getExpression() const {
    return Expression;
  }

  llvm::Optional<Identifier> getName() {
    return Name;
  }

  llvm::Optional<Identifier> getName() const {
    return Name;
  }

  bool hasName() const {
    return Name.has_value();
  }

  bool isExported() const {
    return Exported;
  }

  /// Before function importing get implemented, \c isExternalDeclaration
  /// always returns \c false.
  bool isExternalDeclaration() const {
    return false;
  }

  bool isDefinition() const {
    return !isExternalDeclaration();
  }

  bool isPossiblyUsedExternally() const {
    return isExported();
  }

  DeclContext * getDeclContext() const {
    return getExpression()->getDeclContext();
  }

  StringRef getDescriptiveKindName() const {
    switch (Kind) {
    case FunctionKind::GlobalInit: return "global-init";
    case FunctionKind::Function: return "function";
    }
  }

  std::string getUniqueName() const {
    return (llvm::Twine(getDescriptiveKindName()) + llvm::Twine("$")
            + llvm::Twine(Index))
      .str();
  }

  std::string getUnmangledName() const {
    if (hasName()) {
      return (llvm::Twine(getUniqueName()) + llvm::Twine(" : ")
              + llvm::Twine(getName().value().get()))
        .str();
    }
    return getUniqueName();
  }

  W2N_DEBUG_DUMP;

  void dump(raw_ostream& OS, unsigned Indent = 0) const;
};

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

#endif // W2N_AST_FUNCTION_H
