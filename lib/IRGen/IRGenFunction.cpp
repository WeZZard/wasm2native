#include "IRGenFunction.h"
#include "IRGenModule.h"

using namespace w2n;
using namespace w2n::irgen;

IRGenFunction::IRGenFunction(
  IRGenModule& IGM, Function * Fn, OptimizationMode Mode
) :
  IGM(IGM),
  // FIXME: Derive IRBuilder DebugInfo from IGM & Mode
  Builder(IGM.getLLVMContext(), true),
  OptMode(Mode),
  Fn(Fn) {
}

IRGenFunction::~IRGenFunction() {
}

void IRGenFunction::emitFunction() {
}

void IRGenFunction::unimplemented(SourceLoc Loc, StringRef Message) {
  return IGM.unimplemented(Loc, Message);
}
