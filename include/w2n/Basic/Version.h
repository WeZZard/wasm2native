/// Defines version macros and version-related utility functions for w2n.

#ifndef W2N_BASIC_VERSION_H
#define W2N_BASIC_VERSION_H

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/VersionTuple.h>
#include <array>
#include <string>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class DiagnosticEngine;
class SourceLoc;

namespace version {

/// Represents an internal compiler version, represented as a tuple of
/// integers, or "version components".
///
/// For comparison, if a `CompilerVersion` contains more than one
/// version component, the second one is ignored for comparison,
/// as it represents a compiler variant with no defined ordering.
///
/// A CompilerVersion must have no more than five components and must fit
/// in a 64-bit unsigned integer representation.
///
/// Assuming a maximal version component layout of X.Y.Z.a.b,
/// X, Y, Z, a, b are integers with the following (inclusive) ranges:
/// X: [0 - 9223371]
/// Y: [0 - 999]
/// Z: [0 - 999]
/// a: [0 - 999]
/// b: [0 - 999]
class Version {
  SmallVector<unsigned, 5> Components;

public:
  /// Create the empty compiler version - this always compares greater
  /// or equal to any other CompilerVersion, as in the case of building
  /// w2n from latest sources outside of a build/integration/release
  /// context.
  Version() = default;

  /// Create a literal version from a list of components.
  Version(std::initializer_list<unsigned> Values) : Components(Values) {
  }

  /// Create a version from a string in source code.
  ///
  /// Must include only groups of digits separated by a dot.
  Version(
    StringRef VersionString,
    SourceLoc Loc,
    DiagnosticEngine * Diags
  );

  /// Return a string to be used as an internal preprocessor define.
  ///
  /// The components of the version are multiplied element-wise by
  /// \p componentWeights, then added together (a dot product operation).
  /// If either array is longer than the other, the missing elements are
  /// treated as zero.
  ///
  /// The resulting string will have the form "-DMACRO_NAME=XYYZZ".
  /// The combined value must fit in a uint64_t.
  std::string preprocessorDefinition(
    StringRef macroName,
    ArrayRef<uint64_t> componentWeights
  ) const;

  /// Return the ith version component.
  unsigned operator[](size_t i) const {
    return Components[i];
  }

  /// Return the number of version components.
  size_t size() const {
    return Components.size();
  }

  bool empty() const {
    return Components.empty();
  }

  /// Convert to a (maximum-4-element) llvm::VersionTuple, truncating
  /// away any 5th component that might be in this version.
  operator llvm::VersionTuple() const;

  /// Returns the concrete version to use when \e this version is provided
  /// as an argument to -w2n-version.
  ///
  /// This is not the same as the set of w2n versions that have ever
  /// existed, just those that we are attempting to maintain
  /// backward-compatibility support for. It's also common for valid
  /// versions to produce a different result; for example "-w2n-version 3"
  /// at one point instructed the compiler to act as if it is version 3.1.
  Optional<Version> getEffectiveLanguageVersion() const;

  /// Whether this version is greater than or equal to the given major
  /// version number.
  bool isVersionAtLeast(unsigned major, unsigned minor = 0) const {
    switch (size()) {
    case 0: return false;
    case 1:
      return (
        (Components[0] == major && 0 == minor) || (Components[0] > major)
      );
    default:
      return (
        (Components[0] == major && Components[1] >= minor) ||
        (Components[0] > major)
      );
    }
  }

  /// Return this Version struct with minor and sub-minor components
  /// stripped
  Version asMajorVersion() const;

  /// Return this Version struct as the appropriate version string for
  /// APINotes.
  std::string asAPINotesVersionString() const;

  /// Parse a version in the form used by the _compiler_version \#if
  /// condition.
  static Optional<Version> parseCompilerVersionString(
    StringRef VersionString,
    SourceLoc Loc,
    DiagnosticEngine * Diags
  );

  /// Parse a generic version string of the format [0-9]+(.[0-9]+)*
  ///
  /// Version components can be any unsigned 64-bit number.
  static Optional<Version> parseVersionString(
    StringRef VersionString,
    SourceLoc Loc,
    DiagnosticEngine * Diags
  );

  /// Returns a version from the currently defined W2N_COMPILER_VERSION.
  ///
  /// If W2N_COMPILER_VERSION is undefined, this will return the empty
  /// compiler version.
  static Version getCurrentCompilerVersion();

  /// Returns a version from the currently defined W2N_VERSION_MAJOR and
  /// W2N_VERSION_MINOR.
  static Version getCurrentLanguageVersion();

  // List of backward-compatibility versions that we permit passing as
  // -w2n-version <vers>
  static std::array<StringRef, 3> getValidEffectiveVersions() {
    return {{"4", "4.2", "5"}};
  };
};

bool operator>=(const Version& lhs, const Version& rhs);
bool operator<(const Version& lhs, const Version& rhs);
bool operator==(const Version& lhs, const Version& rhs);

inline bool operator!=(const Version& lhs, const Version& rhs) {
  return !(lhs == rhs);
}

raw_ostream& operator<<(raw_ostream& os, const Version& version);

/// Retrieves the numeric {major, minor} w2n version.
///
/// Note that this is the underlying version of the language, ignoring any
/// -w2n-version flags that may have been used in a particular invocation
/// of the compiler.
std::pair<unsigned, unsigned> getw2nNumericVersion();

/// Retrieves a string representing the complete w2n version, which
/// includes the w2n supported and effective version numbers, the
/// repository version, and the vendor tag.
std::string getw2nFullVersion(
  Version effectiveLanguageVersion = Version::getCurrentLanguageVersion()
);

/// Retrieves the repository revision number (or identifier) from which
/// this w2n was built.
StringRef getw2nRevision();

/// Is the running compiler built with a version tag for distribution?
/// When true, \c Version::getCurrentCompilerVersion returns a valid
/// version and \c getw2nRevision returns the version tuple in string
/// format.
bool isCurrentCompilerTagged();

} // namespace version
} // namespace w2n

#endif // W2N_BASIC_VERSION_H
