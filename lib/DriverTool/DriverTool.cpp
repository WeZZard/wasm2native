#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/StringSaver.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMInitialize.h>
#include <w2n/Driver/FrontendUtil.h>
#include <w2n/DriverTool/DriverTool.h>
#include <w2n/FrontendTool/FrontendTool.h>

using namespace w2n;

static std::string getExecutablePath(const char * FirstArg);

static int runDriver(
  StringRef ExecName,
  ArrayRef<const char *> Argv,
  const ArrayRef<const char *> OriginalArgv
);

int w2n::mainEntry(int argc, const char * argv[]) {
  SmallVector<const char *, 256> ExpandedArgs(&argv[0], &argv[argc]);
  llvm::BumpPtrAllocator Allocator;
  llvm::StringSaver Saver(Allocator);
  driver::ExpandResponseFilesWithRetry(Saver, ExpandedArgs);

  // Initialize the stack trace using the parsed argument vector with
  // expanded response files.

  // PROGRAM_START/InitLLVM overwrites the passed in arguments with UTF-8
  // versions of them on Windows. This also has the effect of overwriting
  // the response file expansion. Since we handle the UTF-8 conversion
  // above, we pass in a copy and throw away the modifications.
  int ThrowawayExpandedArgc = ExpandedArgs.size();
  const char ** ThrowawayExpandedArgv = ExpandedArgs.data();
  PROGRAM_START(ThrowawayExpandedArgc, ThrowawayExpandedArgv);
  ArrayRef<const char *> ExpandedArgv(ExpandedArgs);

  StringRef ExecName = llvm::sys::path::stem(ExpandedArgv[0]);
  ArrayRef<const char *> OriginalArgv(argv, &argv[argc]);
  return runDriver(ExecName, ExpandedArgv, OriginalArgv);
}

int runDriver(
  StringRef ExecName,
  ArrayRef<const char *> Argv,
  const ArrayRef<const char *> OriginalArgv
) {
  if (Argv.size() > 1) {
    StringRef FirstArg(Argv[1]);

    if (FirstArg == "-frontend") {
      return performFrontend(
        llvm::makeArrayRef(Argv.data() + 2, Argv.data() + Argv.size()),
        Argv[0],
        (void *)(intptr_t)getExecutablePath
      );
    }

    if (!FirstArg.startswith("--driver-mode=") && ExecName == "w2n-frontend") {
      return performFrontend(
        llvm::makeArrayRef(Argv.data() + 1, Argv.data() + Argv.size()),
        Argv[0],
        (void *)(intptr_t)getExecutablePath
      );
    }
  }

  // TODO: Non-frontend driver logic
  return 0;
}

std::string getExecutablePath(const char * FirstArg) {
  void * P = (void *)(intptr_t)getExecutablePath;
  return llvm::sys::fs::getMainExecutable(FirstArg, P);
}
