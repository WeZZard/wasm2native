//  This file implements the supporting functions for writing
//  instrumenters of the w2n AST.

#ifndef W2N_AST_DIAGNOSTICSUPPRESSION_H
#define W2N_AST_DIAGNOSTICSUPPRESSION_H

#include <vector>

namespace w2n {

class DiagnosticConsumer;
class DiagnosticEngine;

/// RAII class that suppresses diagnostics by temporarily disabling all of
/// the diagnostic consumers.
class DiagnosticSuppression {
  DiagnosticEngine& Diags;
  std::vector<DiagnosticConsumer *> Consumers;

  DiagnosticSuppression(const DiagnosticSuppression&) = delete;
  DiagnosticSuppression& operator=(const DiagnosticSuppression&) = delete;

public:

  explicit DiagnosticSuppression(DiagnosticEngine& Diags);
  ~DiagnosticSuppression();
  static bool isEnabled(const DiagnosticEngine& Diags);
};

} // namespace w2n

#endif // W2N_AST_DIAGNOSTICSUPPRESSION_H
