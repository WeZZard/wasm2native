/// This file defines data structures to support the request evaluator's
/// automatic incremental dependency tracking functionality.

#ifndef W2N_AST_EVALUATORDEPENDENCIES_H
#define W2N_AST_EVALUATORDEPENDENCIES_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <vector>
#include <w2n/AST/AnyRequest.h>
#include <w2n/AST/DependencyCollector.h>
#include <w2n/AST/RequestCache.h>
#include <w2n/Basic/NullablePtr.h>

namespace w2n {

class SourceFile;

namespace evaluator {

namespace detail {
// Remove this when the compiler bumps to C++17.
template <typename...>
using void_t = void;
} // namespace detail

// A \c DependencySource is currently defined to be a primary source file.
//
// The \c SourceFile instance is an artifact of the current dependency
// system, and should be scrapped if possible. It currently encodes the
// idea that edges in the incremental dependency graph invalidate entire
// files instead of individual contexts.
using DependencySource = w2n::NullablePtr<SourceFile>;

/// A \c DependencyRecorder is an aggregator of named references
/// discovered in a particular \c DependencyScope during the course of
/// request evaluation.
class DependencyRecorder {
  friend DependencyCollector;

  /// Whether we are performing an incremental build and should therefore
  /// record request references.
  bool ShouldRecord;

  /// References recorded while evaluating a dependency source request for
  /// each source file. This map is updated upon completion of a
  /// dependency source request, and includes all references from each
  /// downstream request as well.
  llvm::DenseMap<
    SourceFile *,
    llvm::DenseSet<
      DependencyCollector::Reference,
      DependencyCollector::Reference::Info>>
    FileReferences;

  /// References recorded while evaluating each request. This map is
  /// populated upon completion of each request, and includes all
  /// references from each downstream request as well. Note that uncached
  /// requests don't appear as keys in this map; their references are
  /// charged to the innermost cached active request.
  RequestReferences RequestReferences;

  /// Stack of references from each cached active request. When evaluating
  /// a dependency sink request, we update the innermost set of
  /// references. Upon completion of a request, we union the completed
  /// request's references with the next innermost active request.
  std::vector<llvm::SmallDenseSet<
    DependencyCollector::Reference,
    2,
    DependencyCollector::Reference::Info>>
    ActiveRequestReferences;

#ifndef NDEBUG
  /// Used to catch places where a request's writeDependencySink() method
  /// kicks off another request, which would break invariants, so we
  /// disallow this from happening.
  bool IsRecording = false;
#endif

public:

  DependencyRecorder(bool ShouldRecord) : ShouldRecord(ShouldRecord) {
  }

  /// Push a new empty set onto the activeRequestReferences stack.
  template <typename Request>
  void beginRequest();

  /// Pop the activeRequestReferences stack, and insert recorded
  /// references into the requestReferences map, as well as the next
  /// innermost entry in activeRequestReferences.
  template <typename Request>
  void endRequest(const Request& Req);

  /// When replaying a request whose value has already been cached, we
  /// need to update the innermost set in the activeRequestReferences
  /// stack.
  template <typename Request>
  void replayCachedRequest(const Request& Req);

  /// Upon completion of a dependency source request, we update the
  /// fileReferences map.
  template <typename Request>
  void handleDependencySourceRequest(const Request& Req, SourceFile * SF);

  /// Clear the recorded dependencies of a request, if any.
  template <typename Request>
  void clearRequest(const Request& Req);

private:

  /// Add an entry to the innermost set on the activeRequestReferences
  /// stack. Called from the DependencyCollector.
  void recordDependency(const DependencyCollector::Reference& Ref);

public:

  using ReferenceEnumerator =
    llvm::function_ref<void(const DependencyCollector::Reference&)>;

  /// Enumerates the set of references associated with a given source
  /// file, passing them to the given enumeration callback.
  ///
  /// Only makes sense to call once all dependency sources associated with
  /// this source file have already been evaluated, otherwise the map will
  /// obviously be incomplete.
  ///
  /// The order of enumeration is completely undefined. It is the
  /// responsibility of callers to ensure they are order-invariant or are
  /// sorting the result.
  void enumerateReferencesInFile(
    const SourceFile * SF, ReferenceEnumerator F
  ) const;
};

template <typename Request>
void evaluator::DependencyRecorder::beginRequest() {
  if (!ShouldRecord) {
    return;
  }

  if (!Request::isEverCached && !Request::isDependencySource) {
    return;
  }

  ActiveRequestReferences.push_back({});
}

template <typename Request>
void evaluator::DependencyRecorder::endRequest(const Request& Req) {
  if (!ShouldRecord) {
    return;
  }

  if (!Request::isEverCached && !Request::isDependencySource) {
    return;
  }

  // Grab all the dependencies we've recorded so far, and pop
  // the stack.
  auto Recorded = std::move(ActiveRequestReferences.back());
  ActiveRequestReferences.pop_back();

  // If we didn't record anything, there is nothing to do.
  if (Recorded.empty()) {
    return;
  }

  // Convert the set of dependencies into a vector.
  std::vector<DependencyCollector::Reference> Vec(
    Recorded.begin(), Recorded.end()
  );

  // The recorded dependencies bubble up to the parent request.
  if (!ActiveRequestReferences.empty()) {
    ActiveRequestReferences.back().insert(Vec.begin(), Vec.end());
  }

  // Finally, record the dependencies so we can replay them
  // later when the request is re-evaluated.
  RequestReferences.insert<Request>(std::move(Req), std::move(Vec));
}

template <typename Request>
void evaluator::DependencyRecorder::replayCachedRequest(const Request& Req
) {
  assert(Req.isCached());

  if (!ShouldRecord) {
    return;
  }

  if (ActiveRequestReferences.empty()) {
    return;
  }

  auto Found = RequestReferences.find_as<Request>(Req);
  if (Found == RequestReferences.end<Request>()) {
    return;
  }

  ActiveRequestReferences.back().insert(
    Found->second.begin(), Found->second.end()
  );
}

template <typename Request>
void evaluator::DependencyRecorder::handleDependencySourceRequest(
  const Request& Req, SourceFile * SF
) {
  auto Found = RequestReferences.find_as<Request>(Req);
  if (Found != RequestReferences.end<Request>()) {
    FileReferences[SF].insert(Found->second.begin(), Found->second.end());
  }
}

template <typename Request>
void evaluator::DependencyRecorder::clearRequest(const Request& Req) {
  RequestReferences.erase(Req);
}

} // end namespace evaluator

} // end namespace w2n

#endif // W2N_AST_EVALUATORDEPENDENCIES_H
