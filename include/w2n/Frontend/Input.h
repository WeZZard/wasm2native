#ifndef W2N_FRONTEND_INPUT_H
#define W2N_FRONTEND_INPUT_H

#include <llvm/Support/MemoryBuffer.h>
#include <string>
#include <w2n/Basic/FileTypes.h>
#include <w2n/Basic/InputSpecificPaths.h>
#include <llvm/Support/Path.h>

namespace w2n {

class DiagnosticEngine;

class Input final {
  std::string Filename;
  file_types::ID FileID;
  llvm::MemoryBuffer * Buffer;
  InputSpecificPaths ISPs;

public:
  /// Constructs an input file from the provided data.
  ///
  /// \warning This entrypoint infers the type of the file from its extension
  /// and is therefore not suitable for most clients that use files synthesized
  /// from memory buffers. Use the overload of this constructor accepting a
  /// memory buffer and an explicit \c file_types::ID instead.
  Input(StringRef filename, llvm::MemoryBuffer * buffer = nullptr)
    : Input(
        filename,
        buffer,
        file_types::lookupTypeForExtension(
          llvm::sys::path::extension(filename))) {}

  /// Constructs an input file from the provided data.
  Input(StringRef filename, llvm::MemoryBuffer * buffer, file_types::ID FileID)
    : Filename(
        convertBufferNameFromLLVM_getFileOrSTDIN_toW2NConventions(filename)),
      FileID(FileID), Buffer(buffer) {
    assert(!filename.empty());
  }

public:
  /// Retrieves the type of this input file.
  file_types::ID getType() const { return FileID; };

  /// Retrieves the backing buffer for this input file, if any.
  llvm::MemoryBuffer * getBuffer() const { return Buffer; }

  /// The name of this \c Input, or `-` if this input corresponds to the
  /// standard input stream.
  ///
  /// The returned file name is guaranteed not to be the empty string.
  const std::string& getFileName() const {
    assert(!Filename.empty());
    return Filename;
  }

  /// Return w2n-standard file name from a buffer name set by
  /// llvm::MemoryBuffer::getFileOrSTDIN, which uses "<stdin>" instead of "-".
  static StringRef convertBufferNameFromLLVM_getFileOrSTDIN_toW2NConventions(
    StringRef filename) {
    return filename.equals("<stdin>") ? "-" : filename;
  }

  /// Retrieves the name of the output file corresponding to this input.
  ///
  /// If there is no such corresponding file, the result is the empty string.
  /// If there the resulting output should be directed to the standard output
  /// stream, the result is "-".
  std::string outputFilename() const { return ISPs.OutputFilename; }

  std::string indexUnitOutputFilename() const { return outputFilename(); }

  const InputSpecificPaths& getInputSpecificPaths() const { return ISPs; }

  bool deriveInputSpecificPaths(
    InputSpecificPaths& ISPs,
    DiagnosticEngine& Diag) const;

  void setInputSpecificPaths(InputSpecificPaths&& ISPs) {
    this->ISPs = std::move(ISPs);
  }

  // The next set of functions provides access to those module-specific paths
  // accessed directly from an Input, as opposed to via
  // FrontendInputsAndOutputs. They merely make the call sites
  // a bit shorter. Add more forwarding methods as needed.

  StringRef getDependenciesFilePath() const {
    return getInputSpecificPaths().SupplementaryOutputs.DependenciesFilePath;
  }
  StringRef getSerializedDiagnosticsPath() const {
    return getInputSpecificPaths()
      .SupplementaryOutputs.SerializedDiagnosticsPath;
  }
  StringRef getFixItsOutputPath() const {
    return getInputSpecificPaths().SupplementaryOutputs.FixItsOutputPath;
  }
};

} // namespace w2n

#endif // W2N_FRONTEND_INPUT_H
