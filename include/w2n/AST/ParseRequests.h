#ifndef W2N_AST_PARSE_REQUESTS_H
#define W2N_AST_PARSE_REQUESTS_H

#include <w2n/AST/Decl.h>
#include <w2n/AST/SimpleRequest.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/StableHasher.h>
#include <w2n/Basic/Statistic.h>

namespace w2n {

class Token {};

struct WasmFileParsingResult {
  ArrayRef<Decl *> TopLevelDecls;
  Optional<ArrayRef<Token>> CollectedTokens;
  Optional<StableHasher> InterfaceHasher;
};

/// Parse the top-level decls of a SourceFile.
class ParseWasmFileRequest
  : public SimpleRequest<
      ParseWasmFileRequest,
      WasmFileParsingResult(WasmFile *),
      RequestFlags::SeparatelyCached | RequestFlags::DependencySource> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  WasmFileParsingResult
  evaluate(Evaluator& evaluator, WasmFile * SF) const;

public:
  // Caching.
  bool isCached() const {
    return true;
  }

  Optional<WasmFileParsingResult> getCachedResult() const;
  void cacheResult(WasmFileParsingResult result) const;

public:
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyRecorder&) const;
};

/// The zone number for the parser.
#define W2N_TYPEID_ZONE   Parse
#define W2N_TYPEID_HEADER <w2n/AST/ParseTypeIDZone.def>
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
#include <w2n/AST/ParseTypeIDZone.def>
#undef W2N_REQUEST

void registerParseRequestFunctions(Evaluator& Eval);

} // namespace w2n

#endif // W2N_AST_PARSE_REQUESTS_H
