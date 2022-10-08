#ifndef W2N_OPTION_OPTIONS_H
#define W2N_OPTION_OPTIONS_H

#include <memory>

namespace llvm {
namespace opt {
class OptTable;
}
} // namespace llvm

namespace w2n {
namespace options {
/// Flags specifically for w2n driver options.  Must not overlap with
/// llvm::opt::DriverFlag.
enum W2NFlags {
  FrontendOption = (1 << 4),
  NoDriverOption = (1 << 5),
  NoInteractiveOption = (1 << 6),
  NoBatchOption = (1 << 7),
  ArgumentIsPath = (1 << 8),
};

enum ID {
  OPT_INVALID = 0, // This is not an option ID.

#define OPTION(                                                          \
  PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,         \
  HELPTEXT, METAVAR, VALUES                                              \
)                                                                        \
  OPT_##ID,
#include <w2n/Options/Options.inc>
  LastOption
#undef OPTION
};
} // end namespace options

std::unique_ptr<llvm::opt::OptTable> createW2NOptTable();

} // end namespace w2n

#endif
