#ifndef W2N_DRIVER_DRIVER_H
#define W2N_DRIVER_DRIVER_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {

namespace driver {

namespace sys {
class TaskQueue;
}
class DiagnosticEngine;

class OutputInfo {};

class Driver {

public:
  /// DriverKind determines how later arguments are parsed, as well as the
  /// allowable OutputInfo::Mode values.
  enum class DriverKind {
    Interactive,     // w2n
    Batch,           // w2nc
  };

private:

  DriverKind TheKind;

public:
  Driver(
    StringRef ExeName,
    StringRef Name,
    ArrayRef<const char *> Args,
    DiagnosticEngine& Diags);
  ~Driver();

  DriverKind getDriverKind() const { return TheKind; }
  
};

} // namespace driver

} // namespace w2n

#endif // W2N_DRIVER_DRIVER_H
