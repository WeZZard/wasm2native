/// This file defines a class for recording incremental dependencies.

#ifndef W2N_AST_DEPENDENCYCOLLECTOR_H
#define W2N_AST_DEPENDENCYCOLLECTOR_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <w2n/AST/Identifier.h>

namespace w2n {

class DeclContext;

namespace evaluator {

class DependencyRecorder;

/// A \c DependencyCollector defines an abstract write-only buffer of
/// \c Reference objects. References are added to a collector during the
/// write phase of request evaluation (in \c writeDependencySink) with the
/// various \c add* functions below.
///
/// A \c DependencyCollector should not be created directly; instances are
/// vended by the request evaluator infrastructure itself.
struct DependencyCollector {
  friend DependencyRecorder;

  struct Reference {
  public:

    enum class Kind {
      Empty,
      Tombstone,
      UsedMember,
      PotentialMember,
      TopLevel,
      Dynamic,
    } RefKind;

    DeclContext * Subject;
    DeclBaseName Name;

  private:

    Reference(Kind RefKind, DeclContext * Subject, DeclBaseName Name) :
      RefKind(RefKind),
      Subject(Subject),
      Name(Name) {
    }

  public:

    static Reference empty() {
      return {
        Kind::Empty,
        llvm::DenseMapInfo<DeclContext *>::getEmptyKey(),
        llvm::DenseMapInfo<DeclBaseName>::getEmptyKey()};
    }

    static Reference tombstone() {
      return {
        Kind::Tombstone,
        llvm::DenseMapInfo<DeclContext *>::getTombstoneKey(),
        llvm::DenseMapInfo<DeclBaseName>::getTombstoneKey()};
    }

    static Reference
    usedMember(DeclContext * subject, DeclBaseName Name) {
      return {Kind::UsedMember, subject, Name};
    }

    static Reference potentialMember(DeclContext * Subject) {
      return {Kind::PotentialMember, Subject, DeclBaseName()};
    }

    static Reference topLevel(DeclBaseName Name) {
      return {Kind::TopLevel, nullptr, Name};
    }

    static Reference dynamic(DeclBaseName Name) {
      return {Kind::Dynamic, nullptr, Name};
    }

    struct Info {
      static inline Reference getEmptyKey() {
        return Reference::empty();
      }

      static inline Reference getTombstoneKey() {
        return Reference::tombstone();
      }

      static inline unsigned getHashValue(const Reference& Val) {
        return llvm::hash_combine(
          Val.RefKind, Val.Subject, Val.Name.getAsOpaquePointer()
        );
      }

      static bool isEqual(const Reference& LHS, const Reference& RHS) {
        return LHS.RefKind == RHS.RefKind && LHS.Subject == RHS.Subject
            && LHS.Name == RHS.Name;
      }
    };
  };

private:

  DependencyRecorder& Parent;

public:

  explicit DependencyCollector(DependencyRecorder& Parent);
  ~DependencyCollector();

  /// Registers a named reference from the current dependency scope to a
  /// member defined in the given \p subject type.
  ///
  /// Used member constraints are typically the by-product of direct
  /// lookups, where the name being looked up and the target of the lookup
  /// are known up front. A used member dependency causes the file to be
  /// rebuilt if the definition of that member changes in any way - via
  /// deletion, addition, or mutation of a member with that same name.
  void addUsedMember(DeclContext * Subject, DeclBaseName Name);

  /// Registers a reference from the current dependency scope to a
  /// "potential member" of the given \p subject type.
  ///
  /// A single potential member dependency can be thought of as many used
  /// member dependencies - one for each current member of the subject
  /// type, but also one for every member that will be added or removed
  /// from the type in the future. As such, these dependencies cause
  /// rebuilds when any members are added, removed, or changed in the \p
  /// subject type. It also indicates a dependency on the \p subject
  /// type's existence, so deleting the \p subject type will also cause a
  /// rebuild.
  ///
  /// These dependencies are most appropriate for protocol conformances,
  /// superclass constraints, and other requirements involving entire
  /// types.
  void addPotentialMember(DeclContext * Subject);

  /// Registers a reference from the current dependency scope to a given
  /// top-level \p name.
  ///
  /// A top level dependency causes a rebuild when another top-level
  /// entity with that name is added, removed, or modified.
  void addTopLevelName(DeclBaseName Name);

  /// Registers a reference from the current dependency scope to a given
  /// dynamic member \p name.
  ///
  /// A dynamic lookup dependency is a special kind of member dependency
  /// on a name that is found by \c AnyObject lookup.
  void addDynamicLookupName(DeclBaseName Name);

  /// Retrieves the dependency recorder that created this dependency
  /// collector.
  const DependencyRecorder& getRecorder() const {
    return Parent;
  }
};

} // end namespace evaluator

} // end namespace w2n

#endif // W2N_AST_DEPENDENCYCOLLECTOR_H
