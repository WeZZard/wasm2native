#include <w2n/AST/Type.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;

void Type::dump() const {
  dump(llvm::errs());
}

void Type::dump(raw_ostream& os, unsigned indent) const {
  w2n_proto_implemented([&] { os << "\n"; });
}
