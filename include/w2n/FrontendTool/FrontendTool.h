#ifndef W2N_FRONTENDTOOL_FRONTENDTOOL_H
#define W2N_FRONTENDTOOL_FRONTENDTOOL_H

#include <llvm/ADT/ArrayRef.h>
#include <w2n/Frontend/Frontend.h>

namespace w2n {

/**
 * @brief Performs frontend jobs for a command line invocation.
 *
 * @param args Arguments passed to the frontend
 * @param argv0 The executable name of the invoked program.
 * @param executablePath The executable path of the invoked program.
 * @return \c int the exit value of the frontend: 0 for success, non-zero
 *  for errors.
 *
 */
int performFrontend(
  llvm::ArrayRef<const char *> args,
  const char * argv0,
  void * mainAddr
);

} // namespace w2n

#endif // W2N_FRONTENDTOOL_FRONTENDTOOL_H