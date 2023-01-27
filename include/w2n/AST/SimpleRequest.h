/// This file defines the \c SimpleRequest class template, which makes it
/// easier to define new request kinds.

#ifndef W2N_AST_SIMPLEREQUEST_H
#define W2N_AST_SIMPLEREQUEST_H

#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Error.h>
#include <tuple>
#include <type_traits>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/DiagnosticsCommon.h>
#include <w2n/Basic/LLVMHashing.h>
#include <w2n/Basic/SimpleDisplay.h>
#include <w2n/Basic/Statistic.h>
#include <w2n/Basic/TypeID.h>

namespace w2n {

class Evaluator;

/// Describes how the result for a particular request will be cached.
enum class RequestFlags {
  /// The result for a particular request should never be cached.
  Uncached = 1 << 0,
  /// The result for a particular request should be cached within the
  /// evaluator itself.
  Cached = 1 << 1,
  /// The result of a particular request will be cached via some separate
  /// mechanism, such as a mutable data structure.
  SeparatelyCached = 1 << 2,
  /// This request introduces the source component of a source-sink
  /// incremental dependency pair and defines a new dependency scope.
  ///
  /// This bit is optional.  High-level requests
  /// (e.g. \c TypeCheckSourceFileRequest) will require it.
  ///
  /// For further discussion on incremental dependencies
  /// see DependencyAnalysis.md.
  DependencySource = 1 << 3,
  /// This request introduces the sink component of a source-sink
  /// incremental dependency pair and is a consumer of the current
  /// dependency scope.
  ///
  /// This bit is optional. Name lookup requests
  /// (e.g. \c DirectLookupRequest) will require it.
  ///
  /// For further discussion on incremental dependencies
  /// see DependencyAnalysis.md.
  DependencySink = 1 << 4,
};

static constexpr inline RequestFlags
operator|(RequestFlags Lhs, RequestFlags Rhs) {
  return RequestFlags(
    static_cast<std::underlying_type<RequestFlags>::type>(Lhs)
    | static_cast<std::underlying_type<RequestFlags>::type>(Rhs)
  );
}

/// -------------------------------------------------------------------------
/// Extracting the source location "nearest" a request.
/// -------------------------------------------------------------------------

namespace detail {
/// Dummy extract function used to detect when we can call
/// extractNearestSourceLoc() safely.
inline void extractNearestSourceLoc(...) {
}

/// Metaprogram to determine whether any input is true.
constexpr bool anyTrue() {
  return false;
}

template <typename... RestTy>
constexpr bool anyTrue(bool Current, RestTy... Rest) {
  return Current || anyTrue(Rest...);
}
} // namespace detail

/// Determine whether we can extract a nearest source location from a
/// value of the given type.
template <typename T>
constexpr bool canExtractNearestSourceLoc() {
  using detail::extractNearestSourceLoc;
  return !std::is_void<decltype(extractNearestSourceLoc(*(T *)nullptr)
  )>::value;
}

/// Extract source locations when possible, or return an invalid source
/// location if not possible.
template <
  typename T,
  typename =
    typename std::enable_if<canExtractNearestSourceLoc<T>()>::type>
SourceLoc maybeExtractNearestSourceLoc(const T& Value) {
  return extractNearestSourceLoc(Value);
}

template <
  typename T,
  typename = void,
  typename =
    typename std::enable_if<!canExtractNearestSourceLoc<T>()>::type>
SourceLoc maybeExtractNearestSourceLoc(const T& Value) {
  return SourceLoc();
}

/// Extract the nearest source location from a pointer union.
template <
  typename T,
  typename U,
  typename = typename std::enable_if<
    canExtractNearestSourceLoc<T>()
    && canExtractNearestSourceLoc<U>()>::type>
SourceLoc extractNearestSourceLoc(const llvm::PointerUnion<T, U>& Value) {
  if (auto First = Value.template dyn_cast<T>()) {
    return extractNearestSourceLoc(First);
  }
  if (auto Second = Value.template dyn_cast<U>()) {
    return extractNearestSourceLoc(Second);
  }
  return SourceLoc();
}

namespace detail {
/// Basis case for extracting the nearest source location from a tuple.
template <
  unsigned Index,
  typename... Types,
  typename = void,
  typename = typename std::enable_if<(Index >= sizeof...(Types))>::type>
SourceLoc extractNearestSourceLocTuple(const std::tuple<Types...>& _) {
  return SourceLoc();
}

/// Extract the first, nearest source location from a tuple.
template <
  unsigned Index,
  typename... Types,
  typename = typename std::enable_if<(Index < sizeof...(Types))>::type>
SourceLoc extractNearestSourceLocTuple(const std::tuple<Types...>& Value
) {
  SourceLoc Loc = maybeExtractNearestSourceLoc(std::get<Index>(Value));
  if (Loc.isValid()) {
    return Loc;
  }

  return extractNearestSourceLocTuple<Index + 1>(Value);
}
} // namespace detail

namespace detail {
constexpr bool cacheContains(RequestFlags Kind, RequestFlags Needle) {
  using cache_t = std::underlying_type<RequestFlags>::type;
  return (static_cast<cache_t>(Kind) & static_cast<cache_t>(Needle))
      == static_cast<cache_t>(Needle);
}

constexpr bool isEverCached(RequestFlags Kind) {
  return !cacheContains(Kind, RequestFlags::Uncached);
}

constexpr bool hasExternalCache(RequestFlags Kind) {
  return cacheContains(Kind, RequestFlags::SeparatelyCached);
}

constexpr bool isDependencySource(RequestFlags Kind) {
  return cacheContains(Kind, RequestFlags::DependencySource);
}

constexpr bool isDependencySink(RequestFlags Kind) {
  return cacheContains(Kind, RequestFlags::DependencySink);
}
} // end namespace detail

/// Extract the first, nearest source location from a tuple.
template <
  typename First,
  typename... Rest,
  typename = typename std::enable_if<detail::anyTrue(
    canExtractNearestSourceLoc<First>(),
    canExtractNearestSourceLoc<Rest>()...
  )>::type>
SourceLoc extractNearestSourceLoc(const std::tuple<First, Rest...>& Value
) {
  return detail::extractNearestSourceLocTuple<0>(Value);
}

/// -------------------------------------------------------------------------
/// Simple Requests
/// -------------------------------------------------------------------------

/// CRTP base class that describes a request operation that takes values
/// with the given input types (\c Inputs...) and produces an output of
/// the given type.
///
/// \tparam Derived The final, derived class type for the request.
/// \tparam Signature The signature of the request, described as a
/// function type whose inputs (\c Inputs) are the parameter types and
/// whose output (\c Output) is the result of evaluating this request.
/// \tparam Caching Describes how the output value is cached, if at all.
///
/// The \c Derived class needs to implement several operations. The most
/// important one takes an evaluator and the input values, then computes
/// the final result: \code
///   Output evaluate(Evaluator &evaluator, Inputs...) const;
/// \endcode
///
/// Cycle diagnostics can be handled in one of two ways. Either the \c
/// Derived class can implement the two cycle-diagnosing operations
/// directly: \code
///   void diagnoseCycle(DiagnosticEngine &diags) const;
///   void noteCycleStep(DiagnosticEngine &diags) const;
/// \endcode
///
/// Or the implementation will use the default "circular reference"
/// diagnostics based on the "nearest" source location, which can be
/// provided explicitly by implementing the following in the subclass:
/// \code
///   SourceLoc getNearestLoc() const;
/// \endcode
/// If not provided, a default implementation for \c getNearestLoc() will
/// pick the source location from the first input that provides one.
///
/// Value caching is determined by the \c Caching parameter. When
/// \c Caching == RequestFlags::SeparatelyCached, the \c Derived class is
/// responsible for implementing the two operations responsible to
/// managing the cache: \code
///   Optional<Output> getCachedResult() const;
///   void cacheResult(Output value) const;
/// \endcode
///
/// Incremental dependency tracking occurs automatically during
/// request evaluation. To support that system, high-level requests that
/// define dependency sources should override \c readDependencySource()
/// and specify \c RequestFlags::DependencySource in addition to one of
/// the 3 caching kinds defined above.
/// \code
///   evaluator::DependencySource
///   readDependencySource(const evaluator::DependencyRecorder &) const;
/// \endcode
///
/// Requests that define dependency sinks should instead override
/// \c writeDependencySink() and use the given evaluator and request
/// result to write an edge into the dependency tracker. In addition,
/// \c RequestFlags::DependencySource should be specified along with
/// one of the 3 caching kinds defined above.
/// \code
///   void writeDependencySink(evaluator::DependencyCollector &, Output)
///   const;
/// \endcode
template <typename Derived, typename Signature, RequestFlags Caching>
class SimpleRequest;

template <
  typename Derived,
  RequestFlags Caching,
  typename Output,
  typename... Inputs>
class SimpleRequest<Derived, Output(Inputs...), Caching> {
  std::tuple<Inputs...> Storage;

  Derived& asDerived() {
    return *static_cast<Derived *>(this);
  }

  const Derived& asDerived() const {
    return *static_cast<const Derived *>(this);
  }

  template <size_t... Indices>
  Output
  callDerived(Evaluator& Eval, std::index_sequence<Indices...>) const {
    static_assert(
      sizeof...(Indices) > 0, "Subclass must define evaluate()"
    );
    return asDerived().evaluate(Eval, std::get<Indices>(Storage)...);
  }

protected:

  /// Retrieve the storage value directly.
  const std::tuple<Inputs...>& getStorage() const {
    return Storage;
  }

public:

  constexpr static bool isEverCached = detail::isEverCached(Caching);
  constexpr static bool hasExternalCache =
    detail::hasExternalCache(Caching);

  constexpr static bool isDependencySource =
    detail::isDependencySource(Caching);
  constexpr static bool isDependencySink =
    detail::isDependencySink(Caching);

  using OutputType = Output;

  explicit SimpleRequest(const Inputs&... I) : Storage(I...) {
  }

  /// Request evaluation function that will be registered with the
  /// evaluator.
  static OutputType evaluateRequest(const Derived& Req, Evaluator& Eval) {
    return Req.callDerived(Eval, std::index_sequence_for<Inputs...>());
  }

  /// Retrieve the nearest source location to which this request applies.
  SourceLoc getNearestLoc() const {
    return extractNearestSourceLoc(Storage);
  }

  void diagnoseCycle(DiagnosticEngine& Diags) const {
    Diags.diagnose(asDerived().getNearestLoc(), diag::circular_reference);
  }

  void noteCycleStep(DiagnosticEngine& Diags) const {
    Diags.diagnose(
      asDerived().getNearestLoc(), diag::circular_reference_through
    );
  }

  friend bool
  operator==(const SimpleRequest& Lhs, const SimpleRequest& Rhs) {
    return Lhs.Storage == Rhs.Storage;
  }

  friend bool
  operator!=(const SimpleRequest& Lhs, const SimpleRequest& Rhs) {
    return !(Lhs == Rhs);
  }

  friend llvm::hash_code hash_value(const SimpleRequest& Req) {
    using llvm::hash_combine;
    return hash_combine(Req.Storage);
  }

  friend void simple_display(llvm::raw_ostream& Out, const Derived& Req) {
    Out << TypeID<Derived>::getName();
    simple_display(Out, Req.Storage);
  }

  friend FrontendStatsTracer
  make_tracer(UnifiedStatsReporter * Reporter, const Derived& Request) {
    return make_tracer(
      Reporter, TypeID<Derived>::getName(), Request.Storage
    );
  }
};
} // namespace w2n

namespace llvm {
template <typename T, unsigned N>
llvm::hash_code hash_value(const SmallVector<T, N>& Vec) {
  return hash_combine_range(Vec.begin(), Vec.end());
}
} // namespace llvm

#endif // W2N_BASIC_SIMPLEREQUEST_H
