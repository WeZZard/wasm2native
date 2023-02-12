#ifndef W2N_AST_LOWERING_H
#define W2N_AST_LOWERING_H

#include <w2n/AST/ASTVisitor.h>

namespace w2n {

namespace Lowering {

/// Lowering::ASTVisitor - This is a specialization of w2n::ASTVisitor
/// which works only on resolved nodes and which automatically ignores
/// certain AST node kinds.
template <
  typename ImplClass,
  typename ExprRetTy = void,
  typename StmtRetTy = void,
  typename DeclRetTy = void,
  typename... Args>
class ASTVisitor :
  public w2n::
    ASTVisitor<ImplClass, ExprRetTy, StmtRetTy, DeclRetTy, Args...> {
public:
};

template <typename ImplClass, typename ExprRetTy = void, typename... Args>
using ExprVisitor = ASTVisitor<ImplClass, ExprRetTy, void, void, Args...>;

} // namespace Lowering

} // namespace w2n

#endif // W2N_AST_LOWERING_H
