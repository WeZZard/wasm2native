#ifndef W2N_DRIVER_FRONTENDUTIL_H
#define W2N_DRIVER_FRONTENDUTIL_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/StringSaver.h>
#include <w2n/Basic/LLVM.h>

#include <memory>

namespace w2n {

namespace driver {
/// Expand response files in the argument list with retrying.
/// This function is a wrapper of lvm::cl::ExpandResponseFiles. It will
/// retry calling the function if the previous expansion failed.
void ExpandResponseFilesWithRetry(
  llvm::StringSaver& Saver, llvm::SmallVectorImpl<const char *>& Args
);

} // end namespace driver
} // end namespace w2n

#endif
