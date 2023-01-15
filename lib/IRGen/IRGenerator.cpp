#include <cassert>
#include <w2n/IRGen/IRGenModule.h>
#include <w2n/IRGen/IRGenerator.h>

using namespace w2n;
using namespace w2n::irgen;

IRGenerator::IRGenerator(const IRGenOptions& Opts, ModuleDecl& Module) :
  Opts(Opts),
  Module(Module),
  QueueIndex(0) {
}

void IRGenerator::addGenModule(SourceFile * SF, IRGenModule * IGM) {
  assert(GenModules.count(SF) == 0);
  GenModules[SF] = IGM;
  if (PrimaryIGM == nullptr) {
    PrimaryIGM = IGM;
  }
  Queue.push_back(IGM);
}

IRGenModule * IRGenerator::getGenModule(DeclContext * DC) {
  if (GenModules.size() == 1 || (DC == nullptr)) {
    return getPrimaryIGM();
  }
  SourceFile * SF = DC->getParentSourceFile();
  if (SF == nullptr) {
    return getPrimaryIGM();
  }
  IRGenModule * IGM = GenModules[SF];
  assert(IGM);
  return IGM;
}

IRGenModule * IRGenerator::getGenModule(FuncDecl * F) {
  w2n_unimplemented();
}

void IRGenerator::emitGlobalTopLevel(
  const std::vector<std::string>& LinkerDirectives
) {
  // Generate order numbers for the functions in the SIL module that
  // correspond to definitions in the LLVM module.
  //  Don't bother adding external declarations to the function order.

  // Ensure that relative symbols are collocated in the same LLVM module.

  // for (auto &directive: linkerDirectives) {
  //   createLinkerDirectiveVariable(*PrimaryIGM, directive);
  // }

  // Emit globals.

  assert(PrimaryIGM);

  for (GlobalVariable& V : PrimaryIGM->getWasmModule()->getGlobals()) {
    Decl * D = V.getDecl();
    CurrentIGMPtr IGM =
      getGenModule(D != nullptr ? D->getDeclContext() : nullptr);
    IGM->emitGlobalVariable(&V);
  }

  // Emit SIL functions.

  // Emit static initializers.

  for (auto Iter : *this) {
    IRGenModule * IGM = Iter.second;
    IGM->finishEmitAfterTopLevel();
  }

  emitEntryPointInfo();
}

void IRGenerator::emitEntryPointInfo() {
  w2n_proto_implemented();
}

void IRGenerator::emitCoverageMapping() {
  w2n_proto_implemented();
}

void IRGenerator::emitLazyDefinitions() {
  w2n_proto_implemented();
}
