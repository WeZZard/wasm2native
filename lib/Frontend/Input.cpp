#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <w2n/Frontend/Input.h>

using namespace w2n;

bool Input::derivePrimarySpecificPaths(
  PrimarySpecificPaths& ISPs,
  DiagnosticEngine& Diag
) const {
  switch (FileID) {
  case file_types::ID::TY_Wasm: {
    using namespace llvm::sys;
    StringRef FilenameRef(Filename);
    StringRef Stem = path::stem(Filename);
    StringRef ParentPath = path::parent_path(Filename);

    SmallString<256> FilenameBodyBuf;
    path::append(
      FilenameBodyBuf, path::begin(ParentPath), path::end(ParentPath)
    );
    path::append(FilenameBodyBuf, path::begin(Stem), path::end(Stem));

    Twine FilenameBody(FilenameBodyBuf);

    // Supplementary output paths
    SupplementaryOutputPaths SOPs;
    SOPs.DependenciesFilePath = FilenameBody.concat(".d").str();
    SOPs.SerializedDiagnosticsPath =
      FilenameBody.concat(".serialized-diagnostics").str();
    SOPs.FixItsOutputPath = FilenameBody.concat("-fixit.json").str();
    SOPs.TBDPath = FilenameBody.concat(".tbd").str();

    ISPs =
      PrimarySpecificPaths(FilenameBody.concat(".o").str(), "", SOPs);

    return false;
  }
  case file_types::ID::TY_INVALID:
    // TODO: Diagnostic invalid.
    return true;
  default:
    // TODO: Diagnostic illegal inputs.
    return true;
  }
}
