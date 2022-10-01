#include <w2n/Frontend/Frontend.h>

using namespace w2n;

CompilerInstance::CompilerInstance() {}

bool CompilerInstance::setup(
  const CompilerInvocation& Invocation,
  std::string& Error) {
  this->Invocation = Invocation;
  return true;
}
