#ifndef W2N_SEMA_TYPECHECK_H
#define W2N_SEMA_TYPECHECK_H

#include <w2n/AST/Evaluator.h>

namespace w2n {

namespace TypeCheck {};

void performImportResolution(SourceFile& SF);

void registerTypeCheckerRequestFunctions(Evaluator& evaluator);

} // namespace w2n

#endif // W2N_SEMA_TYPECHECK_H