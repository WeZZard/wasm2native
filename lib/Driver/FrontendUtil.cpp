#include <llvm/ADT/Triple.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <w2n/Driver/FrontendUtil.h>

using namespace w2n;
using namespace w2n::driver;

void w2n::driver::ExpandResponseFilesWithRetry(
  llvm::StringSaver& Saver, llvm::SmallVectorImpl<const char *>& Args
) {
  const unsigned MAX_COUNT = 30;
  for (unsigned I = 0; I != MAX_COUNT; ++I) {
    if (llvm::cl::ExpandResponseFiles(
          Saver,
          llvm::Triple(llvm::sys::getProcessTriple()).isOSWindows()
            ? llvm::cl::TokenizeWindowsCommandLine
            : llvm::cl::TokenizeGNUCommandLine,
          Args
        )) {
      return;
    }
  }
}
