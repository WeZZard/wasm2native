#ifndef W2N_FRONTEND_FRONTENDOPTIONS_H
#define W2N_FRONTEND_FRONTENDOPTIONS_H

#include <llvm/Option/ArgList.h>
#include <w2n/Frontend/FrontendInputsAndOutputs.h>
#include <w2n/Frontend/FrontendOptions.h>
#include <w2n/Frontend/Input.h>

namespace w2n {

/// Options for controlling the behavior of the frontend.
class FrontendOptions {

public:
  enum class ActionType {
    /// @brief Just no action would be taken.
    NoneAction,
    /// @brief Emits object file
    EmitObject,
    /// @brief Emits assmebly file
    EmitAssembly,
    /// @brief Emits LLVM bitcode
    EmitBC,
    /// @brief Emits LLVM IR
    EmitIR,
    /// @brief Emits optimized LLVM IR
    EmitIRGen,
    /// @brief Prints the compiler version.
    PrintVersion,
  };

  FrontendInputsAndOutputs InputsAndOutputs;

  ActionType RequestedAction = ActionType::NoneAction;

  std::string ModuleName;

  std::string ModuleABIName;

  std::string ModuleLinkName;

  unsigned BadFileDescriptorRetryCount = 0;

public:
  FrontendOptions() {}

  static bool doesActionPerformEndOfPipelineActions(ActionType Action);
};

} // namespace w2n

#endif // W2N_FRONTEND_FRONTENDOPTIONS_H
