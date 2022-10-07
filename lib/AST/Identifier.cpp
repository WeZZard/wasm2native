#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/Identifier.h>

using namespace w2n;

raw_ostream& llvm::operator<<(raw_ostream& OS, Identifier I) {
  if (I.get() == nullptr)
    return OS << "_";
  return OS << I.get();
}

int Identifier::compare(Identifier other) const {
  // Handle empty identifiers.
  if (empty() || other.empty()) {
    if (empty() != other.empty()) {
      return other.empty() ? -1 : 1;
    }

    return 0;
  }

  return str().compare(other.str());
}
