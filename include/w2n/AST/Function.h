#ifndef W2N_AST_FUNCTION_H
#define W2N_AST_FUNCTION_H

#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/Decl.h>
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
private:

  llvm::Optional<Identifier> Name;

  FuncTypeDecl * Type;

  /// @note: Global variable's init expression does not have locals.
  std::vector<LocalDecl *> Locals;

  ExpressionDecl * Expression;

  bool Exported;

  Function(
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) :
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
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) {
    return new (Expression->getASTContext())
      Function(Name, Type, Locals, Expression, IsExported);
  }

  static Function * createInit(
    FuncTypeDecl * Type,
    ExpressionDecl * Expression,
    llvm::Optional<Identifier> Name
  ) {
    return createFunction(Name, Type, {}, Expression, false);
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
