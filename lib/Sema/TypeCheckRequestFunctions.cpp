#include <w2n/AST/Module.h>
#include <w2n/AST/TypeCheckerRequests.h>
#include <w2n/Sema/TypeCheck.h>

using namespace w2n;

// Define request evaluation functions for each of the type checker
// requests.
static AbstractRequestFunction * typeCheckerRequestFunctions[] = {
#define W2N_REQUEST(Zone, Name, Sig, Caching, LocOptions)                \
  reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include <w2n/AST/TypeCheckerTypeIDZone.def>
#undef W2N_REQUEST
};

void w2n::registerTypeCheckerRequestFunctions(Evaluator& Eval) {
  Eval.registerRequestFunctions(
    Zone::TypeChecker, typeCheckerRequestFunctions
  );
}
