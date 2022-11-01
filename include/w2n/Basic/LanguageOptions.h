#ifndef W2N_BASIC_LANGUAGEOPTIONS_H
#define W2N_BASIC_LANGUAGEOPTIONS_H

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/VersionTuple.h>

namespace w2n {

class LanguageOptions final {
public:

  /// The target we are building for.
  ///
  /// This represents the minimum deployment target.
  llvm::Triple Target;

  /// The SDK version, if known.
  llvm::Optional<llvm::VersionTuple> SDKVersion;

  /// The SDK canonical name, if known.
  std::string SDKName;

  /// The alternate name to use for the entry point instead of main.
  std::string EntryPointFunctionName = "main";

  bool UsesMalloc = false;

  bool DebugDumpCycles = true;

  bool RecordRequestReferences = true;
};

} // namespace w2n

#endif // W2N_BASIC_LANGUAGEOPTIONS_H