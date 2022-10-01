
#include <w2n/Frontend/FrontendOptions.h>

using namespace w2n;

bool FrontendOptions::doesActionPerformEndOfPipelineActions(ActionType action) {
  switch (action) {
  case ActionType::NoneAction:
  case ActionType::PrintVersion:
    return false;
  case ActionType::EmitAssembly:
  case ActionType::EmitIRGen:
  case ActionType::EmitIR:
  case ActionType::EmitBC:
  case ActionType::EmitObject:
    return true;
  }
  llvm_unreachable("Unknown ActionType");
}