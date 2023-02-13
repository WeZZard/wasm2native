#include <llvm/ADT/ArrayRef.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/ASTTypeIDs.h>
#include <w2n/AST/Evaluator.h>
#include <w2n/AST/ParseRequests.h>
#include <w2n/Basic/Defer.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Parse/Parser.h>

using namespace w2n;

namespace w2n {
// Implement the type checker type zone (zone 10).
#define W2N_TYPEID_ZONE   Parse
#define W2N_TYPEID_HEADER <w2n/AST/ParseTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
} // namespace w2n

//----------------------------------------------------------------------------//
// ParseWasmFileRequest computation.
//----------------------------------------------------------------------------//

WasmFileParsingResult
ParseWasmFileRequest::evaluate(Evaluator& Eval, WasmFile * SF) const {
  assert(SF);
  auto& Ctx = SF->getASTContext();
  auto BufferId = SF->getBufferID();

  // If there's no buffer, there's nothing to parse.
  if (!BufferId) {
    return {};
  }

  // If we've been asked to silence warnings, do so now. This is needed
  // for secondary files, which can be parsed multiple times.
  auto& Diags = Ctx.Diags;
  auto DidSuppressWarnings = Diags.getSuppressWarnings();
  auto ShouldSuppress = SF->getParsingOptions().contains(
    SourceFile::ParsingFlags::SuppressWarnings
  );
  Diags.setSuppressWarnings(DidSuppressWarnings || ShouldSuppress);
  W2N_DEFER {
    Diags.setSuppressWarnings(DidSuppressWarnings);
  };

  auto Parser = WasmParser::createWasmParser(*BufferId, *SF, &Diags);
  // FIXME: PrettyStackTraceParser StackTrace(Parser);

#define W2N_STACK_DECLS_COUNT 128

  SmallVector<Decl *, W2N_STACK_DECLS_COUNT> Decls;
  Parser->parseTopLevel(Decls);

  return WasmFileParsingResult{Ctx.allocateCopy(Decls), None, None};
}

evaluator::DependencySource ParseWasmFileRequest::readDependencySource(
  const evaluator::DependencyRecorder& E
) const {
  return std::get<0>(getStorage());
}

Optional<WasmFileParsingResult>
ParseWasmFileRequest::getCachedResult() const {
  auto * SF = std::get<0>(getStorage());
  auto Decls = SF->getCachedTopLevelDecls();
  if (!Decls) {
    return None;
  }

  return WasmFileParsingResult{*Decls, None, None};
}

void ParseWasmFileRequest::cacheResult(WasmFileParsingResult Result
) const {
  auto * SF = std::get<0>(getStorage());
  assert(!SF->hasCachedTopLevelDecls());
  SF->setCachedTopLevelDecls(std::vector<Decl *>(Result.TopLevelDecls));

  // Verify the parsed source file.
  // FIXME: verify(*SF);
}

//----------------------------------------------------------------------------//
// Parse Request Functions Registration
//----------------------------------------------------------------------------//

// Define request evaluation functions for each of the type checker
// requests.
static AbstractRequestFunction * ParseRequestFunctions[] = {
#define W2N_REQUEST(Zone, Name, Sig, Caching, LocOptions)                \
  reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include <w2n/AST/ParseTypeIDZone.def>
#undef W2N_REQUEST
};

void w2n::registerParseRequestFunctions(Evaluator& Eval) {
  Eval.registerRequestFunctions(Zone::Parse, ParseRequestFunctions);
}
