#ifndef W2N_AST_IRGENOPTIONS_H
#define W2N_AST_IRGENOPTIONS_H

#include <llvm/ADT/SmallVector.h>
#include <string>
#include <vector>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/Basic/OptimizationMode.h>

namespace w2n {

enum class IRGenOutputKind : unsigned {
  /// Just generate an LLVM module and return it.
  Module,

  /// Generate an LLVM module and write it out as LLVM assembly.
  LLVMAssemblyBeforeOptimization,

  /// Generate an LLVM module and write it out as LLVM assembly.
  LLVMAssemblyAfterOptimization,

  /// Generate an LLVM module and write it out as LLVM bitcode.
  LLVMBitcode,

  /// Generate an LLVM module and compile it to assembly.
  NativeAssembly,

  /// Generate an LLVM module, compile it, and assemble into an object
  /// file.
  ObjectFile
};

enum class IRGenLLVMLTOKind : unsigned {
  None,
  Thin,
  Full,
};

enum class IRGenEmbedMode : unsigned {
  None,
  EmbedMarker,
  EmbedBitcode
};

class IRGenOptions {
public:

  /// The libraries and frameworks specified on the command line.
  SmallVector<LinkLibrary, 4> LinkLibraries;

  /// The public dependent libraries specified on the command line.
  std::vector<std::string> PublicLinkLibraries;

  /// If non-empty, the (unmangled) name of a dummy symbol to emit that
  /// can be used to force-load this module.
  std::string ForceLoadSymbolName;

  /// The kind of compilation we should do.
  IRGenOutputKind OutputKind : 3;

  /// Should we spend time verifying that the IR we produce is
  /// well-formed?
  unsigned Verify : 1;

  /// Whether we should embed the bitcode file.
  IRGenEmbedMode EmbedMode : 2;

  IRGenLLVMLTOKind LLVMLTOKind : 2;

  OptimizationMode OptMode;

  unsigned EnableGlobalISel : 1;

  /// Emit functions to separate sections.
  unsigned FunctionSections : 1;

  bool shouldOptimize() const {
    return OptMode > OptimizationMode::NoOptimization;
  }
};

} // namespace w2n

#endif