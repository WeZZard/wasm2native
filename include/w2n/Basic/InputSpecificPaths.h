
#ifndef W2N_BASIC_MODULESPECIFICPATHS_H
#define W2N_BASIC_MODULESPECIFICPATHS_H

#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SupplementaryOutputPaths.h>

namespace w2n {

/**
 * @brief Holds all of the output paths, and debugging-info path that are
 * specific to which module is being compiled at the moment.
 */
class InputSpecificPaths {
public:
  /// The name of the main output file,
  /// that is, the .o file for this input (or a file specified by -o).
  /// If there is no such file, contains an empty string. If the output
  /// is to be written to stdout, contains "-".
  std::string OutputFilename;

  SupplementaryOutputPaths SupplementaryOutputs;

  InputSpecificPaths(
    std::string OutputFilename = std::string(),
    SupplementaryOutputPaths SupplementaryOutputs = SupplementaryOutputPaths())
    : OutputFilename(OutputFilename),
      SupplementaryOutputs(SupplementaryOutputs) {}
};

} // namespace w2n

#endif // W2N_BASIC_MODULESPECIFICPATHS_H