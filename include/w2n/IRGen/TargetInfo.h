#ifndef W2N_IRGEN_TARGETINFO_H
#define W2N_IRGEN_TARGETINFO_H

#include <llvm/ADT/Triple.h>

namespace w2n {

class TargetInfo {

public:
  const llvm::Triple& getTriple() const;
  const char * getDataLayoutString() const;
};

} // namespace w2n

#endif // W2N_IRGEN_TARGETINFO_H