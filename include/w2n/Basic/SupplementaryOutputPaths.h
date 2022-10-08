#ifndef W2N_FRONTEND_SUPPLEMENTARYOUTPUTPATHS_H
#define W2N_FRONTEND_SUPPLEMENTARYOUTPUTPATHS_H

#include <llvm/IR/Function.h>
#include <w2n/Basic/LLVM.h>

#include <string>

namespace w2n {
struct SupplementaryOutputPaths {

  /// The path to which we should output a Make-style dependencies file.
  /// It is valid whenever there are any inputs.
  ///
  /// w2n's compilation model means that Make-style dependencies can only
  /// model WebAssembly's import section.
  std::string DependenciesFilePath;

  /// Path to a file which should contain serialized diagnostics for this
  /// frontend invocation.
  ///
  /// This uses the same serialized diagnostics format as Clang, for tools
  /// that want machine-parseable diagnostics.
  ///
  /// \sa w2n::serialized_diagnostics::createConsumer
  std::string SerializedDiagnosticsPath;

  /// The path to which we should output fix-its as source edits.
  ///
  /// This is a JSON-based format that is used by the migrator, but is not
  /// really vetted for anything else.
  ///
  /// \sa w2n::writeEditsInJson
  std::string FixItsOutputPath;

  /// The path to which we should output a TBD file.
  ///
  /// "TBD" stands for "text-based dylib". It's a YAML-based format that
  /// describes the public ABI of a library, which clients can link
  /// against without having an actual dynamic library binary.
  ///
  /// Only makes sense when the compiler has whole-module knowledge.
  ///
  /// \sa w2n::writeTBDFile
  std::string TBDPath;

  /// The output path to generate ABI baseline.
  // TODO: std::string ABIDescriptorOutputPath;

  /// The output path for YAML optimization record file.
  // TODO: std::string YAMLOptRecordPath;

  /// The output path for bitstream optimization record file.
  // TODO: std::string BitstreamOptRecordPath;

  SupplementaryOutputPaths() = default;

  /// Apply a given function for each existing (non-empty string)
  /// supplementary output
  void forEachSetOutput(llvm::function_ref<void(const std::string&)> fn
  ) const {
    if (!DependenciesFilePath.empty())
      fn(DependenciesFilePath);
    if (!SerializedDiagnosticsPath.empty())
      fn(SerializedDiagnosticsPath);
    if (!FixItsOutputPath.empty())
      fn(FixItsOutputPath);
    if (!TBDPath.empty())
      fn(TBDPath);
  }

  bool empty() const {
    return DependenciesFilePath.empty() &&
           SerializedDiagnosticsPath.empty() &&
           FixItsOutputPath.empty() && TBDPath.empty();
  }
};
} // namespace w2n

#endif // W2N_FRONTEND_SUPPLEMENTARYOUTPUTPATHS_H
