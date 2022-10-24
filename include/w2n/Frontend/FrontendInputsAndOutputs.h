#ifndef W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H
#define W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H

#include <llvm/ADT/StringMap.h>
#include <w2n/Frontend/Input.h>

namespace w2n {

/// A set of frontend inputs and outputs.
class FrontendInputsAndOutputs final {

  std::vector<Input> AllInputs;
  llvm::StringMap<unsigned> PrimaryInputsByName;
  std::vector<unsigned> PrimaryInputsInOrder;

  /// Recover missing inputs. Note that recovery itself is users
  /// responsibility.
  bool ShouldRecoverMissingInputs = false;

  bool IsSingleThreadedWMO = false;

public:
  FrontendInputsAndOutputs() = default;
  FrontendInputsAndOutputs(const FrontendInputsAndOutputs& other);
  FrontendInputsAndOutputs&
  operator=(const FrontendInputsAndOutputs& other);

  bool shouldRecoverMissingInputs() const {
    return ShouldRecoverMissingInputs;
  }

  void setShouldRecoverMissingInputs() {
    ShouldRecoverMissingInputs = true;
  }

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
  // Primaries:

  const Input& firstPrimaryInput() const;
  const Input& lastPrimaryInput() const;

  /// If \p fn returns true, exit early and return true.
  bool forEachPrimaryInput(llvm::function_ref<bool(const Input&)> fn
  ) const;

  /// Iterates over primary inputs, exposing their unique ordered index
  /// If \p fn returns true, exit early and return true.
  bool forEachPrimaryInputWithIndex(
    llvm::function_ref<bool(const Input&, unsigned index)> fn
  ) const;

  /// If \p fn returns true, exit early and return true.
  bool forEachNonPrimaryInput(llvm::function_ref<bool(const Input&)> fn
  ) const;

  unsigned primaryInputCount() const {
    return PrimaryInputsInOrder.size();
  }

  // Primary count readers:

  bool hasUniquePrimaryInput() const { return primaryInputCount() == 1; }

  bool hasPrimaryInputs() const { return primaryInputCount() > 0; }

  bool hasMultiplePrimaryInputs() const {
    return primaryInputCount() > 1;
  }

#pragma mark - Mutating

#pragma mark Inputs

public:
  void clearInputs();
  void addInput(const Input& input);
  void
  addInputFile(StringRef file, llvm::MemoryBuffer * buffer = nullptr);
  void addPrimaryInputFile(
    StringRef file,
    llvm::MemoryBuffer * buffer = nullptr
  );

  bool isSingleThreadedWMO() const { return IsSingleThreadedWMO; }

  void setIsSingleThreadedWMO(bool istw) { IsSingleThreadedWMO = istw; }

  const PrimarySpecificPaths& getPrimarySpecificPathsForPrimary(StringRef
  ) const;

  /// Under single-threaded WMO, we pretend that the first input
  /// generates the main output, even though it will include code
  /// generated from all of them.
  ///
  /// If \p fn returns true, return early and return true.
  bool forEachInputProducingAMainOutputFile(
    llvm::function_ref<bool(const Input&)> fn
  ) const;

  std::vector<std::string> copyOutputFilenames() const;
  std::vector<std::string> copyIndexUnitOutputFilenames() const;

  void forEachOutputFilename(llvm::function_ref<void(StringRef)> fn
  ) const;
};

} // namespace w2n

#endif // W2N_FRONTEND_FRONTENDINPUTSANDOUTPUTS_H
