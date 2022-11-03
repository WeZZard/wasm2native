#include <w2n/Basic/Unimplemented.h>

void w2n::details::report_prototype_implementation(
  const char * file, unsigned line, const char * reason
) {
  if (reason) {
    llvm::dbgs() << reason << "\n";
  }
  llvm::dbgs() << "Prototype implementation";
  if (file) {
    llvm::dbgs() << " at " << file << ":" << line;
  }
  llvm::dbgs() << ".\n";
}
