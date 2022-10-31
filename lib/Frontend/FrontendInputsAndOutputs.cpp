#include <w2n/Frontend/FrontendInputsAndOutputs.h>
#include <w2n/Frontend/Input.h>

using namespace w2n;

FrontendInputsAndOutputs::FrontendInputsAndOutputs(
  const FrontendInputsAndOutputs& other
) {
  for (Input input : other.AllInputs)
    addInput(input);
}

FrontendInputsAndOutputs&
FrontendInputsAndOutputs::operator=(const FrontendInputsAndOutputs& other
) {
  clearInputs();
  for (Input input : other.AllInputs)
    addInput(input);
  return *this;
}

// All inputs:

std::vector<std::string>
FrontendInputsAndOutputs::getInputFilenames() const {
  std::vector<std::string> filenames;
  for (auto& input : AllInputs) {
    filenames.push_back(input.getFileName());
  }
  return filenames;
}

bool FrontendInputsAndOutputs::isReadingFromStdin() const {
  return hasSingleInput() && getFilenameOfFirstInput() == "-";
}

const std::string&
FrontendInputsAndOutputs::getFilenameOfFirstInput() const {
  assert(hasInputs());
  const Input& input = AllInputs[0];
  const std::string& f = input.getFileName();
  assert(!f.empty());
  return f;
}

bool FrontendInputsAndOutputs::forEachInput(
  llvm::function_ref<bool(const Input&)> fn
) const {
  for (const Input& input : AllInputs)
    if (fn(input))
      return true;
  return false;
}

// Primaries:

const Input& FrontendInputsAndOutputs::firstPrimaryInput() const {
  assert(!PrimaryInputsInOrder.empty());
  return AllInputs[PrimaryInputsInOrder.front()];
}

const Input& FrontendInputsAndOutputs::lastPrimaryInput() const {
  assert(!PrimaryInputsInOrder.empty());
  return AllInputs[PrimaryInputsInOrder.back()];
}

bool FrontendInputsAndOutputs::forEachPrimaryInput(
  llvm::function_ref<bool(const Input&)> fn
) const {
  for (unsigned i : PrimaryInputsInOrder)
    if (fn(AllInputs[i]))
      return true;
  return false;
}

bool FrontendInputsAndOutputs::forEachPrimaryInputWithIndex(
  llvm::function_ref<bool(const Input&, unsigned index)> fn
) const {
  for (unsigned i : PrimaryInputsInOrder)
    if (fn(AllInputs[i], i))
      return true;
  return false;
}

bool FrontendInputsAndOutputs::forEachNonPrimaryInput(
  llvm::function_ref<bool(const Input&)> fn
) const {
  return forEachInput([&](const Input& f) -> bool {
    return f.isPrimary() ? false : fn(f);
  });
}

void FrontendInputsAndOutputs::clearInputs() {
  AllInputs.clear();
  PrimaryInputsByName.clear();
  PrimaryInputsInOrder.clear();
}

void FrontendInputsAndOutputs::addInput(const Input& input) {
  const unsigned index = AllInputs.size();
  AllInputs.push_back(input);
  if (input.isPrimary()) {
    PrimaryInputsInOrder.push_back(index);
    PrimaryInputsByName.insert({AllInputs.back().getFileName(), index});
  }
}

void FrontendInputsAndOutputs::addInputFile(
  StringRef file, llvm::MemoryBuffer * buffer
) {
  addInput(Input(file, false, buffer));
}

void FrontendInputsAndOutputs::addPrimaryInputFile(
  StringRef file, llvm::MemoryBuffer * buffer
) {
  addInput(Input(file, true, buffer));
}

const PrimarySpecificPaths&
FrontendInputsAndOutputs::getPrimarySpecificPathsForPrimary(
  StringRef filename
) const {
  const Input * f = primaryInputNamed(filename);
  return f->getPrimarySpecificPaths();
}

const Input * FrontendInputsAndOutputs::primaryInputNamed(StringRef name
) const {
  assert(!name.empty() && "input files have names");
  StringRef correctedFile =
    Input::convertBufferNameFromLLVM_getFileOrSTDIN_toW2NConventions(name
    );
  auto iterator = PrimaryInputsByName.find(correctedFile);
  if (iterator == PrimaryInputsByName.end())
    return nullptr;
  const Input * f = &AllInputs[iterator->second];
  assert(
    f->isPrimary() && "PrimaryInputsByName should only include primaries"
  );
  return f;
}

bool FrontendInputsAndOutputs::forEachInputProducingAMainOutputFile(
  llvm::function_ref<bool(const Input&)> fn
) const {
  return isSingleThreadedWMO() ? fn(firstInput())
       : hasPrimaryInputs()    ? forEachPrimaryInput(fn)
                               : forEachInput(fn);
}

std::vector<std::string>
FrontendInputsAndOutputs::copyOutputFilenames() const {
  std::vector<std::string> outputs;
  (void
  )forEachInputProducingAMainOutputFile([&](const Input& input) -> bool {
    outputs.push_back(input.outputFilename());
    return false;
  });
  return outputs;
}

std::vector<std::string>
FrontendInputsAndOutputs::copyIndexUnitOutputFilenames() const {
  std::vector<std::string> outputs;
  (void
  )forEachInputProducingAMainOutputFile([&](const Input& input) -> bool {
    outputs.push_back(input.indexUnitOutputFilename());
    return false;
  });
  return outputs;
}

void FrontendInputsAndOutputs::forEachOutputFilename(
  llvm::function_ref<void(StringRef)> fn
) const {
  (void
  )forEachInputProducingAMainOutputFile([&](const Input& input) -> bool {
    fn(input.outputFilename());
    return false;
  });
}