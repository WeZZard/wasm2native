#ifndef W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H
#define W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H

#include <w2n/Frontend/Input.h>

namespace w2n {

/// A set of frontend inputs and outputs.
class FrontendInputsAndOutputs final {

  std::vector<Input> AllInputs;

public:
  FrontendInputsAndOutputs() = default;
  FrontendInputsAndOutputs(const FrontendInputsAndOutputs& other);
  FrontendInputsAndOutputs& operator=(const FrontendInputsAndOutputs& other);

#pragma mark - Reading

#pragma mark Inputs

public:
  ArrayRef<Input> getAllInputs() const { return AllInputs; }

  std::vector<std::string> getInputFilenames() const;

  /// \return nullptr if not a primary input file.
  const Input * primaryInputNamed(StringRef name) const;

  unsigned inputCount() const { return AllInputs.size(); }

  bool hasInputs() const { return !AllInputs.empty(); }

  bool hasSingleInput() const { return inputCount() == 1; }

  const Input& firstInput() const { return AllInputs[0]; }
  Input& firstInput() { return AllInputs[0]; }

  const Input& lastInput() const { return AllInputs.back(); }

  const std::string& getFilenameOfFirstInput() const;

  bool isReadingFromStdin() const;

  /// If \p fn returns true, exits early and returns true.
  bool forEachInput(llvm::function_ref<bool(const Input&)> fn) const;

#pragma mark - Mutating

#pragma mark Inputs

public:
  void clearInputs();
  void addInput(const Input& input);
  void addInputFile(StringRef file, llvm::MemoryBuffer * buffer = nullptr);
};

} // namespace w2n

#endif // W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H
