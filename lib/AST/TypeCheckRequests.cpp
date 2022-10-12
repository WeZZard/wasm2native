#include <w2n/AST/TypeCheckerRequests.h>

using namespace w2n;

namespace w2n {
// Implement the type checker type zone (zone 10).
#define W2N_TYPEID_ZONE TypeChecker
#define W2N_TYPEID_HEADER <w2n/AST/TypeCheckerTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
}
