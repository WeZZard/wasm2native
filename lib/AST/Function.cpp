#include <_types/_uint32_t.h>
#include <w2n/AST/Function.h>

using namespace w2n;

void Function::dump(raw_ostream& OS, unsigned Indent) const {
  OS << getDescriptiveKindName().str().data();
  OS << " $" << getIndex();
  auto Name = getName();
  if (Name.has_value()) {
    OS << " " << Name.value();
  }
  OS << " : ";
  Type->getType()->dump(OS);
}
