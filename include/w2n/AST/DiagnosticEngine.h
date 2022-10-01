#ifndef W2N_AST_DIAGNOSTICENGINE_H
#define W2N_AST_DIAGNOSTICENGINE_H

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SaveAndRestore.h>
#include <llvm/Support/VersionTuple.h>

namespace w2n {

class SourceManager;

enum class DiagID;

/// Describes the kind of diagnostic argument we're storing.
///
enum class DiagnosticArgumentKind {
  String,
  Integer,
  Unsigned,
  Identifier,
  // ... types in wasm file
  Diagnostic,
};

class DiagnosticEngine {
public:
  /// The source manager used to interpret source locations and
  /// display diagnostics.
  SourceManager& SourceMgr;

public:
  explicit DiagnosticEngine(SourceManager& SourceMgr)
    : SourceMgr(SourceMgr) {}

};

} // namespace w2n

#endif // W2N_AST_DIAGNOSTICENGINE_H