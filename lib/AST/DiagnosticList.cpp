/// This file defines all of the diagnostics emitted by Swift.

#include <w2n/AST/DiagnosticsCommon.h>
using namespace w2n;

enum class w2n::DiagID : uint32_t {
#define DIAG(KIND, ID, Options, Text, Signature) ID,
#include <w2n/AST/DiagnosticsAll.def>
};
static_assert(
  static_cast<uint32_t>(w2n::DiagID::invalid_diagnostic) == 0,
  "0 is not the invalid diagnostic ID"
);

enum class w2n::FixItID : uint32_t {
#define DIAG(KIND, ID, Options, Text, Signature)
#define FIXIT(ID, Text, Signature) ID,
#include <w2n/AST/DiagnosticsAll.def>
};

// Define all of the diagnostic objects and initialize them with their
// diagnostic IDs.
namespace w2n {
namespace diag {
#define DIAG(KIND, ID, Options, Text, Signature)                         \
  detail::DiagWithArguments<void Signature>::Type ID = {DiagID::ID};
#define FIXIT(ID, Text, Signature)                                       \
  detail::StructuredFixItWithArguments<void Signature>::Type ID = {      \
    FixItID::ID};
#include <w2n/AST/DiagnosticsAll.def>
} // end namespace diag
} // end namespace w2n
