#ifndef W2N_AST_FUNCTION_H
#define W2N_AST_FUNCTION_H

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/ilist.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTWalker.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Type.h>

namespace w2n {
class ASTContext;

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

  ModuleDecl * Module;

  FunctionKind Kind;

  uint32_t Index;

  llvm::Optional<Identifier> Name;

  /// FIXME: \c FuncTypeDecl is weird here.
  /// When we creating function for global inits, it is weird to create
  /// a \c FuncTypeDecl in-place.
  FuncTypeDecl * Type;

  /// @note: Global variable's init expression does not have locals.
  std::vector<LocalDecl *> Locals;

  ExpressionDecl * Expression;

  bool Exported;

  Function(
    ModuleDecl * Module,
    FunctionKind Kind,
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) :
    Module(Module),
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
    ModuleDecl * Module,
    uint32_t Index,
    llvm::Optional<Identifier> Name,
    FuncTypeDecl * Type,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression,
    bool IsExported
  ) {
    return new (Expression->getASTContext()) Function(
      Module,
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
    ModuleDecl * Module,
    uint32_t Index,
    ValueType * ReturnType,
    ExpressionDecl * Expression,
    llvm::Optional<Identifier> Name
  );

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

  ASTContext& getASTContext() const {
    return getExpression()->getASTContext();
  }

  ModuleDecl * getModule() {
    return Module;
  }

  const ModuleDecl * getModule() const {
    return Module;
  }

  StringRef getDescriptiveKindName() const {
    switch (Kind) {
    case FunctionKind::GlobalInit: return "global-init";
    case FunctionKind::Function: return "function";
    }
  }

  /// A name used for debugging the compiler like: global-init$0,
  /// global-init$1 or function$0, funciton$1 ...
  std::string getDescriptiveName() const;

  /// A full qualified name used for debugging the compiler like:
  /// module.global-init$0, module.global-init$1 ...
  std::string getFullQualifiedDescriptiveName() const;

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
