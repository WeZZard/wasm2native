
#include <w2n/Frontend/FrontendOptions.h>

using namespace w2n;

bool FrontendOptions::doesActionPerformEndOfPipelineActions(
  ActionType Action
) {
  switch (Action) {
  case ActionType::NoneAction:
  case ActionType::PrintVersion: return false;
  case ActionType::EmitAssembly:
  case ActionType::EmitIRGen:
  case ActionType::EmitIR:
  case ActionType::EmitBC:
  case ActionType::EmitObject: return true;
  }
  llvm_unreachable("Unknown ActionType");
}

const PrimarySpecificPaths&
FrontendOptions::getPrimarySpecificPathsForPrimary(
  StringRef PrimaryFilename
) const {
  return InputsAndOutputs.getPrimarySpecificPathsForPrimary(
    PrimaryFilename
  );
}
