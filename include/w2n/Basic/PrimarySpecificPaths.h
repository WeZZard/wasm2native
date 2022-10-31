
#ifndef W2N_BASIC_PRIMARYSPECIFICPATHS_H
#define W2N_BASIC_PRIMARYSPECIFICPATHS_H

#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SupplementaryOutputPaths.h>

namespace w2n {

/**
 * @brief Holds all of the output paths, and debugging-info path that are
 * specific to which module is being compiled at the moment.
 */
class PrimarySpecificPaths {
public:
  /// The name of the main output file,
  /// that is, the .o file for this input (or a file specified by -o).
  /// If there is no such file, contains an empty string. If the output
  /// is to be written to stdout, contains "-".
  std::string OutputFilename;

  SupplementaryOutputPaths SupplementaryOutputs;

  /// The name of the "main" input file, used by the debug info.
  std::string MainInputFilenameForDebugInfo;

  PrimarySpecificPaths(
    std::string OutputFilename = std::string(),
    StringRef MainInputFilenameForDebugInfo = StringRef(),
    SupplementaryOutputPaths SupplementaryOutputs =
      SupplementaryOutputPaths()
  ) :
    OutputFilename(OutputFilename),
    SupplementaryOutputs(SupplementaryOutputs),
    MainInputFilenameForDebugInfo(MainInputFilenameForDebugInfo) {
  }
};

} // namespace w2n

#endif // W2N_BASIC_PRIMARYSPECIFICPATHS_H