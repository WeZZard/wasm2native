#ifndef W2N_AST_EVALUATOR_H
#define W2N_AST_EVALUATOR_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <w2n/AST/AnyRequest.h>
#include <w2n/AST/EvaluatorDependencies.h>
#include <w2n/AST/RequestCache.h>
#include <w2n/Basic/AnyValue.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/Statistic.h>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace w2n {

using llvm::ArrayRef;
using llvm::None;
using llvm::Optional;

class DiagnosticEngine;
class Evaluator;
class UnifiedStatsReporter;

/// An "abstract" request function pointer, which is the storage type
/// used for each of the
using AbstractRequestFunction = void(void);

/// Form the specific request function for the given request type.
template <typename Request>
using RequestFunction =
  typename Request::OutputType(const Request&, Evaluator&);

/// Pretty stack trace handler for an arbitrary request.
template <typename Request>
class PrettyStackTraceRequest : public llvm::PrettyStackTraceEntry {
  const Request& Req;

public:

  PrettyStackTraceRequest(const Request& Req) : Req(Req) {
  }

  void print(llvm::raw_ostream& out) const override {
    out << "While evaluating request ";
    simple_display(out, Req);
    out << "\n";
  }
};

/// An llvm::ErrorInfo container for a request in which a cycle was
/// detected and diagnosed.
template <typename Request>
struct CyclicalRequestError :
  public llvm::ErrorInfo<CyclicalRequestError<Request>> {
public:

  static char ID;
  const Request& Req;
  const Evaluator& Eval;

  CyclicalRequestError(const Request& Req, const Evaluator& Eval) :
    Req(Req),
    Eval(Eval) {
  }

  virtual void log(llvm::raw_ostream& out) const override;

  virtual std::error_code convertToErrorCode() const override {
    // This is essentially unused, but is a temporary requirement for
    // llvm::ErrorInfo subclasses.
    llvm_unreachable(
      "shouldn't get std::error_code from CyclicalRequestError"
    );
  }
};

template <typename Request>
char CyclicalRequestError<Request>::ID = '\0';

/// Evaluates a given request or returns a default value if a cycle is
/// detected.
template <typename Request>
typename Request::OutputType evaluateOrDefault(
  Evaluator& Eval, Request Req, typename Request::OutputType DefaultValue
) {
  auto Result = Eval(Req);
  if (auto Err = Result.takeError()) {
    llvm::handleAllErrors(
      std::move(Err),
      [](const CyclicalRequestError<Request>& E) {
        // cycle detected
      }
    );
    return DefaultValue;
  }
  return *Result;
}

/// Report that a request of the given kind is being evaluated, so it
/// can be recorded by the stats reporter.
template <typename Request>
void reportEvaluatedRequest(
  UnifiedStatsReporter& Stats, const Request& Req
) {
}

/// Evaluation engine that evaluates and caches "requests", checking for
/// cyclic dependencies along the way.
///
/// Each request is a function object that accepts a reference to the
/// evaluator itself (through which it can request other values) and
/// produces a value. That value can then be cached by the evaluator for
/// subsequent access, using a policy dictated by the request itself.
///
/// The evaluator keeps track of all in-flight requests so that it can
/// detect and diagnose cyclic dependencies.
///
/// Each request should be its own function object, supporting the
/// following API:
///
///   - Copy constructor
///   - Equality operator (==)
///   - Hashing support (hash_value)
///   - TypeID support (see swift/Basic/TypeID.h)
///   - The output type (described via a nested type OutputType), which
///     must itself by a value type that supports TypeID.
///   - Evaluation via the function call operator:
///       OutputType operator()(Evaluator &evaluator) const;
///   - Cycle breaking and diagnostics operations:
///
///       void diagnoseCycle(DiagnosticEngine &diags) const;
///       void noteCycleStep(DiagnosticEngine &diags) const;
///   - Caching policy:
///
///     static const bool isEverCached;
///
///       When false, the request's result will never be cached. When
///       true, the result will be cached on completion. How it is cached
///       depends on the following.
///
///     bool isCached() const;
///
///       Dynamically indicates whether to cache this particular instance
///       of the request, so that (for example) requests for which a quick
///       check usually suffices can avoid caching a trivial result.
///
///     static const bool hasExternalCache;
///
///       When false, the results will be cached within the evaluator and
///       cannot be accessed except through the evaluator. This is the
///       best approach, because it ensures that all accesses to the
///       result are tracked.
///
///       When true, the request itself must provide an way to cache the
///       results, e.g., in some external data structure. External caching
///       should only be used when staging in the use of the evaluator
///       into existing mutable data structures; new computations should
///       not depend on it. Externally-cached requests must provide
///       additional API:
///
///         Optional<OutputType> getCachedResult() const;
///
///           Retrieve the cached result, or \c None if there is no such
///           result.
///
///         void cacheResult(OutputType value) const;
///
///            Cache the given result.
class Evaluator {
  /// The diagnostics engine through which any cyclic-dependency
  /// diagnostics will be emitted.
  DiagnosticEngine& Diags;

  /// Whether to dump detailed debug info for cycles.
  bool DebugDumpCycles;

  /// Used to report statistics about which requests were evaluated, if
  /// non-null.
  UnifiedStatsReporter * Stats = nullptr;

  /// A vector containing the abstract request functions that can compute
  /// the result of a particular request within a given zone. The
  /// \c uint8_t is the zone number of the request, and the array is
  /// indexed by the index of the request type within that zone. Each
  /// entry is a function pointer that will be reinterpret_cast'd to
  ///
  ///   RequestType::OutputType (*)(const RequestType &request,
  ///                               Evaluator &evaluator);
  /// and called to satisfy the request.
  std::vector<std::pair<uint8_t, ArrayRef<AbstractRequestFunction *>>>
    RequestFunctionsByZone;

  /// A vector containing all of the active evaluation requests, which
  /// is treated as a stack and is used to detect cycles.
  llvm::SetVector<ActiveRequest> ActiveRequests;

  /// A cache that stores the results of requests.
  evaluator::RequestCache Cache;

  evaluator::DependencyRecorder Recorder;

  /// Retrieve the request function for the given zone and request IDs.
  AbstractRequestFunction *
  getAbstractRequestFunction(uint8_t ZoneID, uint8_t RequestID) const;

  /// Retrieve the request function for the given request type.
  template <typename Request>
  auto getRequestFunction() const -> RequestFunction<Request> * {
    auto AbstractFn = getAbstractRequestFunction(
      TypeID<Request>::zoneID, TypeID<Request>::localID
    );
    assert(AbstractFn && "No request function for request");
    return reinterpret_cast<RequestFunction<Request> *>(AbstractFn);
  }

public:

  /// Construct a new evaluator that can emit cyclic-dependency
  /// diagnostics through the given diagnostics engine.
  Evaluator(DiagnosticEngine& Diags, const LanguageOptions& Opts);

  /// Emit GraphViz output visualizing the request graph.
  void emitRequestEvaluatorGraphViz(llvm::StringRef GraphVizPath);

  /// Set the unified stats reporter through which evaluated-request
  /// statistics will be recorded.
  void setStatsReporter(UnifiedStatsReporter * Stats) {
    this->Stats = Stats;
  }

  /// Register the set of request functions for the given zone.
  ///
  /// These functions will be called to evaluate any requests within that
  /// zone.
  void registerRequestFunctions(
    Zone Zone, ArrayRef<AbstractRequestFunction *> Functions
  );

  void enumerateReferencesInFile(
    const SourceFile * SF,
    evaluator::DependencyRecorder::ReferenceEnumerator F
  ) const {
    return Recorder.enumerateReferencesInFile(SF, F);
  }

  /// Retrieve the result produced by evaluating a request that can
  /// be cached.
  template <
    typename Request,
    typename std::enable_if<Request::isEverCached>::type * = nullptr>
  llvm::Expected<typename Request::OutputType>
  operator()(const Request& Req) {
    // The request can be cached, but check a predicate to determine
    // whether this particular instance is cached. This allows more
    // fine-grained control over which instances get cache.
    if (Req.isCached()) {
      return getResultCached(Req);
    }

    return getResultUncached(Req);
  }

  /// Retrieve the result produced by evaluating a request that
  /// will never be cached.
  template <
    typename Request,
    typename std::enable_if<!Request::isEverCached>::type * = nullptr>
  llvm::Expected<typename Request::OutputType>
  operator()(const Request& Req) {
    return getResultUncached(Req);
  }

  /// Evaluate a set of requests and return their results as a tuple.
  ///
  /// Use this to describe cases where there are multiple (known)
  /// requests that all need to be satisfied.
  template <typename... Requests>
  std::tuple<llvm::Expected<typename Requests::OutputType>...>
  operator()(const Requests&... Reqs) {
    return std::tuple<llvm::Expected<typename Requests::OutputType>...>(
      (*this)(Reqs)...
    );
  }

  /// Cache a precomputed value for the given request, so that it will not
  /// be computed.
  template <
    typename Request,
    typename std::enable_if<Request::hasExternalCache>::type * = nullptr>
  void
  cacheOutput(const Request& Req, typename Request::OutputType&& Output) {
    Req.cacheResult(std::move(Output));
  }

  /// Cache a precomputed value for the given request, so that it will not
  /// be computed.
  template <
    typename Request,
    typename std::enable_if<!Request::hasExternalCache>::type * = nullptr>
  void
  cacheOutput(const Request& Req, typename Request::OutputType&& Output) {
    Cache.insert<Request>(Req, std::move(Output));
  }

  /// Do not introduce new callers of this function.
  template <
    typename Request,
    typename std::enable_if<!Request::hasExternalCache>::type * = nullptr>
  void clearCachedOutput(const Request& Req) {
    Cache.erase<Request>(Req);
    Recorder.clearRequest<Request>(Req);
  }

  /// Clear the cache stored within this evaluator.
  ///
  /// Note that this does not clear the caches of requests that use
  /// external caching.
  void clearCache() {
    Cache.clear();
  }

  /// Is the given request, or an equivalent, currently being evaluated?
  template <typename Request>
  bool hasActiveRequest(const Request& Req) const {
    return ActiveRequests.count(ActiveRequest(Req));
  }

private:

  /// Diagnose a cycle detected in the evaluation of the given
  /// request.
  void diagnoseCycle(const ActiveRequest& Req);

  /// Check the dependency from the current top of the stack to
  /// the given request, including cycle detection and diagnostics.
  ///
  /// \returns true if a cycle was detected, in which case this function
  /// has already diagnosed the cycle. Otherwise, returns \c false and
  /// adds this request to the \c activeRequests stack.
  bool checkDependency(const ActiveRequest& Req);

  /// Produce the result of the request without caching.
  template <typename Request>
  llvm::Expected<typename Request::OutputType>
  getResultUncached(const Request& Req) {
    auto ActiveReq = ActiveRequest(Req);

    // Check for a cycle.
    if (checkDependency(ActiveReq)) {
      return llvm::Error(
        std::make_unique<CyclicalRequestError<Request>>(Req, *this)
      );
    }

    PrettyStackTraceRequest<Request> PrettyStackTrace(Req);

    // TODO: statistics
    // FrontendStatsTracer statsTracer = make_tracer(stats, request);
    // if (stats) reportEvaluatedRequest(*stats, request);

    Recorder.beginRequest<Request>();

    auto&& Result = getRequestFunction<Request>()(Req, *this);

    Recorder.endRequest<Request>(Req);

    handleDependencySourceRequest<Request>(Req);
    handleDependencySinkRequest<Request>(Req, Result);

    // Make sure we remove this from the set of active requests once we're
    // done.
    assert(ActiveRequests.back() == ActiveReq);
    ActiveRequests.pop_back();

    return std::move(Result);
  }

  /// Get the result of a request, consulting an external cache
  /// provided by the request to retrieve previously-computed results
  /// and detect recursion.
  template <
    typename Request,
    typename std::enable_if<Request::hasExternalCache>::type * = nullptr>
  llvm::Expected<typename Request::OutputType>
  getResultCached(const Request& Req) {
    // If there is a cached result, return it.
    if (auto Cached = Req.getCachedResult()) {
      Recorder.replayCachedRequest(Req);
      handleDependencySinkRequest<Request>(Req, *Cached);
      return *Cached;
    }

    // Compute the result.
    auto Result = getResultUncached(Req);

    // Cache the result if applicable.
    if (!Result) {
      return Result;
    }

    Req.cacheResult(*Result);

    // Return it.
    return Result;
  }

  /// Get the result of a request, consulting the general cache to
  /// retrieve previously-computed results and detect recursion.
  template <
    typename Request,
    typename std::enable_if<!Request::hasExternalCache>::type * = nullptr>
  llvm::Expected<typename Request::OutputType>
  getResultCached(const Request& Req) {
    // If we already have an entry for this request in the cache, return
    // it.
    auto Known = Cache.find_as<Request>(Req);
    if (Known != Cache.end<Request>()) {
      auto Result = Known->second;
      Recorder.replayCachedRequest(Req);
      handleDependencySinkRequest<Request>(Req, Result);
      return Result;
    }

    // Compute the result.
    auto Result = getResultUncached(Req);
    if (!Result) {
      return Result;
    }

    // Cache the result.
    Cache.insert<Request>(Req, *Result);
    return Result;
  }

  template <
    typename Request,
    typename std::enable_if<!Request::isDependencySink>::type * = nullptr>
  void handleDependencySinkRequest(
    const Request& Req, const typename Request::OutputType& Output
  ) {
  }

  template <
    typename Request,
    typename std::enable_if<Request::isDependencySink>::type * = nullptr>
  void handleDependencySinkRequest(
    const Request& Req, const typename Request::OutputType& Output
  ) {
    evaluator::DependencyCollector Collector(Recorder);
    Req.writeDependencySink(Collector, Output);
  }

  template <
    typename Request,
    typename std::enable_if<!Request::isDependencySource>::type * =
      nullptr>
  void handleDependencySourceRequest(const Request& Req) {
  }

  template <
    typename Request,
    typename std::enable_if<Request::isDependencySource>::type * =
      nullptr>
  void handleDependencySourceRequest(const Request& Req) {
    auto Source = Req.readDependencySource(Recorder);
    if (!Source.isNull() && Source.get()->isPrimary()) {
      Recorder.handleDependencySourceRequest(Req, Source.get());
    }
  }
};

template <typename Request>
void CyclicalRequestError<Request>::log(llvm::raw_ostream& out) const {
  out << "Cycle detected:\n";
  simple_display(out, Req);
  out << "\n";
}

} // namespace w2n

#endif // W2N_AST_EVALUATOR_H
