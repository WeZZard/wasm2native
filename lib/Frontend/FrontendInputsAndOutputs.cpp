#include <w2n/Frontend/Input.h>
#include <w2n/Frontend/FrontendInputsAndOutputs.h>

using namespace w2n;

FrontendInputsAndOutputs::FrontendInputsAndOutputs(
  const FrontendInputsAndOutputs& other) {
  for (Input input : other.AllInputs)
    addInput(input);
}

FrontendInputsAndOutputs&
FrontendInputsAndOutputs::operator=(const FrontendInputsAndOutputs& other) {
  clearInputs();
  for (Input input : other.AllInputs)
    addInput(input);
  return *this;
}

// All inputs:

std::vector<std::string> FrontendInputsAndOutputs::getInputFilenames() const {
  std::vector<std::string> filenames;
  for (auto& input : AllInputs) {
    filenames.push_back(input.getFileName());
  }
  return filenames;
}

bool FrontendInputsAndOutputs::isReadingFromStdin() const {
  return hasSingleInput() && getFilenameOfFirstInput() == "-";
}

const std::string& FrontendInputsAndOutputs::getFilenameOfFirstInput() const {
  assert(hasInputs());
  const Input& input = AllInputs[0];
  const std::string& f = input.getFileName();
  assert(!f.empty());
  return f;
}

bool FrontendInputsAndOutputs::forEachInput(
  llvm::function_ref<bool(const Input&)> fn) const {
  for (const Input& input : AllInputs)
    if (fn(input))
      return true;
  return false;
}

void FrontendInputsAndOutputs::clearInputs() { AllInputs.clear(); }

void FrontendInputsAndOutputs::addInput(const Input& input) {
  AllInputs.push_back(input);
}

void FrontendInputsAndOutputs::addInputFile(
  StringRef file,
  llvm::MemoryBuffer * buffer) {
  addInput(Input(file, buffer));
}
