//===--- IRGenRequests.h - IRGen Requests -----------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
//  This file defines IRGen requests.
//
//===----------------------------------------------------------------===//

#ifndef W2N_AST_IRGENREQUESTS_H
#define W2N_AST_IRGENREQUESTS_H

#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Target/TargetMachine.h>
#include <w2n/AST/ASTTypeIDs.h>
#include <w2n/AST/EvaluatorDependencies.h>
#include <w2n/AST/SimpleRequest.h>
#include <w2n/Basic/AnyValue.h>
#include <w2n/Basic/PrimarySpecificPaths.h>
#include <w2n/Basic/Statistic.h>

namespace w2n {
class FileUnit;
class SourceFile;
class IRGenOptions;
struct TBDGenOptions;
class TBDGenDescriptor;

namespace irgen {
class IRGenModule;
} // namespace irgen

} // namespace w2n

namespace llvm {
class GlobalVariable;
class LLVMContext;
class Module;
class TargetMachine;

namespace orc {
class ThreadSafeModule;
} // namespace orc

} // namespace llvm

namespace w2n {

/// A pair consisting of an \c LLVMContext and an \c llvm::Module that
/// enforces exclusive ownership of those resources, and ensures that they
/// are deallocated or transferred together.
///
/// Note that the underlying module and context are either both valid
/// pointers or both null. This class forbids the remaining cases by
/// construction.
class GeneratedModule final {
private:

  std::unique_ptr<llvm::LLVMContext> Context;
  std::unique_ptr<llvm::Module> Module;
  std::unique_ptr<llvm::TargetMachine> Target;

  GeneratedModule() : Context(nullptr), Module(nullptr), Target(nullptr) {
  }

  GeneratedModule(GeneratedModule const&) = delete;
  GeneratedModule& operator=(GeneratedModule const&) = delete;

public:

  /// Construct a \c GeneratedModule that owns a given module and context.
  ///
  /// The given pointers must not be null. If a null \c GeneratedModule is
  /// needed, use \c GeneratedModule::null() instead.
  explicit GeneratedModule(
    std::unique_ptr<llvm::LLVMContext>&& Context,
    std::unique_ptr<llvm::Module>&& Module,
    std::unique_ptr<llvm::TargetMachine>&& Target
  ) :
    Context(std::move(Context)),
    Module(std::move(Module)),
    Target(std::move(Target)) {
    assert(getModule() && "Use GeneratedModule::null() instead");
    assert(getContext() && "Use GeneratedModule::null() instead");
    assert(getTargetMachine() && "Use GeneratedModule::null() instead");
  }

  GeneratedModule(GeneratedModule&&) = default;
  GeneratedModule& operator=(GeneratedModule&&) = default;

  /// Construct a \c GeneratedModule that does not own any resources.
  static GeneratedModule null() {
    return GeneratedModule{};
  }

  explicit operator bool() const {
    return Module != nullptr && Context != nullptr;
  }

  const llvm::Module * getModule() const {
    return Module.get();
  }

  llvm::Module * getModule() {
    return Module.get();
  }

  const llvm::LLVMContext * getContext() const {
    return Context.get();
  }

  llvm::LLVMContext * getContext() {
    return Context.get();
  }

  const llvm::TargetMachine * getTargetMachine() const {
    return Target.get();
  }

  llvm::TargetMachine * getTargetMachine() {
    return Target.get();
  }

  /// Release ownership of the context and module to the caller, consuming
  /// this value in the process.
  ///
  /// The REPL is the only caller that needs this. New uses of this
  /// function should be avoided at all costs.
  std::pair<llvm::LLVMContext *, llvm::Module *> release() && {
    return {Context.release(), Module.release()};
  }

  /// Transfers ownership of the underlying module and context to an
  /// ORC-compatible context.
  llvm::orc::ThreadSafeModule intoThreadSafeContext() &&;
};

struct IRGenDescriptor {
  llvm::PointerUnion<FileUnit *, ModuleDecl *> Ctx;

  using SymsToEmit = Optional<llvm::SmallVector<std::string, 1>>;
  SymsToEmit SymbolsToEmit;

  const IRGenOptions& Opts;
  const TBDGenOptions& TBDOpts;

  ModuleDecl& Mod;

  StringRef ModuleName;
  const PrimarySpecificPaths& PSPs;

  ArrayRef<std::string> ParallelOutputFilenames;
  llvm::GlobalVariable ** OutModuleHash;

  friend llvm::hash_code hash_value(const IRGenDescriptor& hs) {
    return llvm::hash_combine(hs.Ctx, hs.SymbolsToEmit);
  }

  friend bool
  operator==(const IRGenDescriptor& Lhs, const IRGenDescriptor& Rhs) {
    return Lhs.Ctx == Rhs.Ctx && Lhs.SymbolsToEmit == Rhs.SymbolsToEmit;
  }

  friend bool
  operator!=(const IRGenDescriptor& Lhs, const IRGenDescriptor& Rhs) {
    return !(Lhs == Rhs);
  }

public:

  static IRGenDescriptor forFile(
    FileUnit * File,
    const IRGenOptions& Opts,
    const TBDGenOptions& TBDOpts,
    ModuleDecl& Mod,
    StringRef ModuleName,
    const PrimarySpecificPaths& PSPs,
    SymsToEmit SymbolsToEmit = None,
    llvm::GlobalVariable ** OutModuleHash = nullptr
  ) {
    return IRGenDescriptor{
      File,
      SymbolsToEmit,
      Opts,
      TBDOpts,
      Mod,
      ModuleName,
      PSPs,
      {},
      OutModuleHash};
  }

  static IRGenDescriptor forWholeModule(
    ModuleDecl& WholeModule,
    const IRGenOptions& Opts,
    const TBDGenOptions& TBDOpts,
    StringRef ModuleName,
    const PrimarySpecificPaths& PSPs,
    SymsToEmit SymbolsToEmit = None,
    ArrayRef<std::string> ParallelOutputFilenames = {},
    llvm::GlobalVariable ** OutModuleHash = nullptr
  ) {
    return IRGenDescriptor{
      &WholeModule,
      SymbolsToEmit,
      Opts,
      TBDOpts,
      WholeModule, // FIXME: verify this usage
      ModuleName,
      PSPs,
      ParallelOutputFilenames,
      OutModuleHash};
  }

  /// Retrieves the files to perform IR generation for. If the descriptor
  /// is configured only to emit a specific set of symbols, this will be
  /// empty.
  TinyPtrVector<FileUnit *> getFilesToEmit() const;

  /// For a single file, returns its parent module, otherwise returns the
  /// module itself.
  ModuleDecl * getParentModule() const;

  /// Retrieve a descriptor suitable for generating TBD for the file or
  /// module.
  TBDGenDescriptor getTBDGenDescriptor() const;

  /// Compute the linker directives to emit.
  std::vector<std::string> getLinkerDirectives() const;
};

/// Report that a request of the given kind is being evaluated, so it
/// can be recorded by the stats reporter.
template <typename Request>
void reportEvaluatedRequest(
  UnifiedStatsReporter& Stats, const Request& Req
);

class IRGenRequest :
  public SimpleRequest<
    IRGenRequest,
    GeneratedModule(IRGenDescriptor),
    RequestFlags::Uncached | RequestFlags::DependencySource> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  GeneratedModule evaluate(Evaluator& Eval, IRGenDescriptor Desc) const;

public:

  // Incremental dependencies.
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyRecorder&) const;
};

void simple_display(llvm::raw_ostream& os, const IRGenDescriptor& ss);

SourceLoc extractNearestSourceLoc(const IRGenDescriptor& Desc);

/// Returns the optimized IR for a given file or module. Note this runs
/// the entire compiler pipeline and ignores the passed SILModule.
class OptimizedIRRequest :
  public SimpleRequest<
    OptimizedIRRequest,
    GeneratedModule(IRGenDescriptor),
    RequestFlags::Uncached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  GeneratedModule evaluate(Evaluator& Eval, IRGenDescriptor Desc) const;
};

using SymbolsToEmit = SmallVector<std::string, 1>;

/// Return the object code for a specific set of symbols in a file or
/// module.
class SymbolObjectCodeRequest :
  public SimpleRequest<
    SymbolObjectCodeRequest,
    StringRef(IRGenDescriptor),
    RequestFlags::Cached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  StringRef evaluate(Evaluator& Eval, IRGenDescriptor Desc) const;

public:

  // Caching.
  bool isCached() const {
    return true;
  }
};

/// The zone number for IRGen.
#define W2N_TYPEID_ZONE   IRGen
#define W2N_TYPEID_HEADER <w2n/AST/IRGenTypeIDZone.def>
#include <w2n/Basic/DefineTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define W2N_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)         \
  template <>                                                            \
  inline void reportEvaluatedRequest(                                    \
    UnifiedStatsReporter& stats, const RequestType& request              \
  ) {                                                                    \
    ++stats.getFrontendCounters().RequestType;                           \
  }
#include <w2n/AST/IRGenTypeIDZone.def>
#undef W2N_REQUEST

/// Register IRGen-level request functions with the evaluator.
///
/// Clients that form an ASTContext and will perform any IR generation
/// should call this functions after forming the ASTContext.
void registerIRGenRequestFunctions(Evaluator& Eval);

} // namespace w2n

#endif // W2N_AST_IRGENREQUESTS_H
