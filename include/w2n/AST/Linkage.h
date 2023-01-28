#ifndef W2N_AST_LINKAGE_H
#define W2N_AST_LINKAGE_H

#include <cstdint>

namespace w2n {

// FIXME: Is it enough to offer just two AST-level linkages?
enum class ASTLinkage : uint8_t {
  Public,
  Internal,
};

} // namespace w2n

#endif // W2N_AST_LINKAGE_H