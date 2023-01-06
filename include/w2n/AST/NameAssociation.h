#ifndef W2N_AST_NAMEASSOCIATION_H
#define W2N_AST_NAMEASSOCIATION_H

#include <cstdint>
#include <vector>
#include <w2n/AST/Identifier.h>

namespace w2n {

struct NameAssociation {
  uint32_t Index;

  Identifier Name;
};

struct IndirectNameAssociation {
  uint32_t Index;

  std::vector<NameAssociation> NameMap;
};

} // namespace w2n

#endif // W2N_AST_NAMEASSOCIATION_H