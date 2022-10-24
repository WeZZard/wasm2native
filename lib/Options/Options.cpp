#include <w2n/Options/Options.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/Option/Option.h>
#include <llvm/Option/OptTable.h>

using namespace w2n::options;
using namespace llvm::opt;

#define PREFIX(NAME, VALUE) static const char * const NAME[] = VALUE;
#include <w2n/Options/Options.inc>
#undef PREFIX

static const OptTable::Info InfoTable[] = {
#define OPTION(                                                          \
  PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,         \
  HELPTEXT, METAVAR, VALUES                                              \
)                                                                        \
  {PREFIX,      NAME,      HELPTEXT,                                     \
   METAVAR,     OPT_##ID,  Option::KIND##Class,                          \
   PARAM,       FLAGS,     OPT_##GROUP,                                  \
   OPT_##ALIAS, ALIASARGS, VALUES},
#include <w2n/Options/Options.inc>
#undef OPTION
};

namespace {

class W2NOptTable : public OptTable {
public:
  W2NOptTable() : OptTable(InfoTable) {}
};

} // end anonymous namespace

std::unique_ptr<OptTable> w2n::createW2NOptTable() {
  return std::unique_ptr<OptTable>(new W2NOptTable());
}
