/// This file defines diagnostics for the frontend.

#ifndef W2N_DIAGNOSTICSFRONTEND_H
#define W2N_DIAGNOSTICSFRONTEND_H

#include <w2n/AST/DiagnosticsCommon.h>

namespace w2n {
  namespace diag {
  // Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND,ID,Options,Text,Signature) \
  extern detail::DiagWithArguments<void Signature>::type ID;
#include <w2n/AST/DiagnosticsFrontend.def>
  }
}

#endif
