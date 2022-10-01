#ifndef W2N_AST_ASTCONTEXT_H
#define W2N_AST_ASTCONTEXT_H

namespace w2n {

/**
 * @brief We still can use a tree representation for wasm files, since this
 * enables verification and easier CFG building.
 *
 */
class ASTContext {

public:

ASTContext();

bool hadError() const { return false; }

};

class ModuleDecl {};

} // namespace w2n

#endif // W2N_AST_ASTCONTEXT_H