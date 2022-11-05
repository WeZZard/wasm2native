#include <llvm/Support/Debug.h>
#include <vector>
#include <w2n/Basic/Unimplemented.h>

void w2n::details::report_prototype_implementation(
  const char * File, unsigned Line, const char * Reason
) {
  if (Reason != nullptr) {
    llvm::dbgs() << Reason << "\n";
  }
  llvm::dbgs() << "Prototype implementation";
  if (File != nullptr) {
    llvm::dbgs() << " at " << File << ":" << Line;
  }
  llvm::dbgs() << ".\n";
}

void w2n::details::report_assertion(
  const char * Function,
  const char * File,
  int Line,
  const char * Expression,
  const char * Fmt,
  ...
) {
  if (Fmt != nullptr) {
    va_list Args1;
    va_start(Args1, Fmt);
    va_list Args2;
    va_copy(Args2, Args1);
    std::vector<char> Buf(1 + std::vsnprintf(nullptr, 0, Fmt, Args1));
    va_end(Args1);
    std::vsnprintf(Buf.data(), Buf.size(), Fmt, Args2);
    va_end(Args2);
    llvm::dbgs() << Buf.data() << ". ";
  }
  if (Expression != nullptr) {
    llvm::dbgs() << Expression << " ";
  }
  llvm::dbgs() << "assertion failure: ";
  if (File != nullptr) {
    llvm::dbgs() << " at " << File << ":" << Line;
  }
  if (Function != nullptr) {
    llvm::dbgs() << " function: " << Function;
  }
  llvm::dbgs() << ".\n";
  abort();
}