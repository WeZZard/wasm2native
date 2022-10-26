#ifndef W2N_FRONTEND_INPUT_H
#define W2N_FRONTEND_INPUT_H

#include <llvm/ADT/PointerIntPair.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <string>
#include <w2n/Basic/FileTypes.h>
#include <w2n/Basic/PrimarySpecificPaths.h>

namespace w2n {

class DiagnosticEngine;

class Input final {
  std::string Filename;
  file_types::ID FileID;
  llvm::PointerIntPair<llvm::MemoryBuffer *, 1, bool> BufferAndIsPrimary;
  PrimarySpecificPaths ISPs;

public:
  /// Constructs an input file from the provided data.
  ///
  /// \warning This entrypoint infers the type of the file from its
  /// extension and is therefore not suitable for most clients that use
  /// files synthesized from memory buffers. Use the overload of this
  /// constructor accepting a memory buffer and an explicit \c
  /// file_types::ID instead.
  Input(
    StringRef Filename,
    bool IsPrimary,
    llvm::MemoryBuffer * Buffer = nullptr
  )
    : Input(
        Filename,
        IsPrimary,
        Buffer,
        file_types::lookupTypeForExtension(
          llvm::sys::path::extension(Filename)
        )
      ) {
  }

  /// Constructs an input file from the provided data.
  Input(
    StringRef Filename,
    bool IsPrimary,
    llvm::MemoryBuffer * Buffer,
    file_types::ID FileID
  )
    : Filename(convertBufferNameFromLLVM_getFileOrSTDIN_toW2NConventions(
        Filename
      )),
      FileID(FileID), BufferAndIsPrimary(Buffer, IsPrimary) {
    assert(!Filename.empty());
  }

public:
  /// Retrieves the type of this input file.
  file_types::ID getType() const {
    return FileID;
  };

  /// Retrieves whether this input file was passed as a primary to the
  /// frontend.
  bool isPrimary() const {
    return BufferAndIsPrimary.getInt();
  }

  /// Retrieves the backing buffer for this input file, if any.
  llvm::MemoryBuffer * getBuffer() const {
    return BufferAndIsPrimary.getPointer();
  }

  /// The name of this \c Input, or `-` if this input corresponds to the
  /// standard input stream.
  ///
  /// The returned file name is guaranteed not to be the empty string.
  const std::string& getFileName() const {
    assert(!Filename.empty());
    return Filename;
  }

  /// Return w2n-standard file name from a buffer name set by
  /// llvm::MemoryBuffer::getFileOrSTDIN, which uses "<stdin>" instead of
  /// "-".
  static StringRef
  convertBufferNameFromLLVM_getFileOrSTDIN_toW2NConventions(
    StringRef filename
  ) {
    return filename.equals("<stdin>") ? "-" : filename;
  }

  /// Retrieves the name of the output file corresponding to this input.
  ///
  /// If there is no such corresponding file, the result is the empty
  /// string. If there the resulting output should be directed to the
  /// standard output stream, the result is "-".
  std::string outputFilename() const {
    return ISPs.OutputFilename;
  }

  std::string indexUnitOutputFilename() const {
    return outputFilename();
  }

  const PrimarySpecificPaths& getPrimarySpecificPaths() const {
    return ISPs;
  }

  bool derivePrimarySpecificPaths(
    PrimarySpecificPaths& ISPs,
    DiagnosticEngine& Diag
  ) const;

  void setPrimarySpecificPaths(PrimarySpecificPaths&& ISPs) {
    this->ISPs = std::move(ISPs);
  }

  // The next set of functions provides access to those module-specific
  // paths accessed directly from an Input, as opposed to via
  // FrontendInputsAndOutputs. They merely make the call sites
  // a bit shorter. Add more forwarding methods as needed.

  StringRef getDependenciesFilePath() const {
    return getPrimarySpecificPaths()
      .SupplementaryOutputs.DependenciesFilePath;
  }

  StringRef getSerializedDiagnosticsPath() const {
    return getPrimarySpecificPaths()
      .SupplementaryOutputs.SerializedDiagnosticsPath;
  }

  StringRef getFixItsOutputPath() const {
    return getPrimarySpecificPaths()
      .SupplementaryOutputs.FixItsOutputPath;
  }
};

} // namespace w2n

#endif // W2N_FRONTEND_INPUT_H
