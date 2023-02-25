#include "IRGenFunction.h"
#include "IRGenModule.h"
#include "Reduction.h"
#include <w2n/AST/Lowering.h>

using namespace w2n;
using namespace w2n::irgen;

#pragma mark - RValueEmitter

namespace {

class RValueEmitter :
  public Lowering::ExprVisitor<RValueEmitter, RValue> {
public:

  Function * Fn;

  IRGenModule& IGM;

  IRBuilder& Builder;

  Configuration& Config;

  RValueEmitter(
    Function * Fn,
    IRGenModule& IGM,
    IRBuilder& Builder,
    Configuration& Config
  ) :
    Fn(Fn),
    IGM(IGM),
    Builder(Builder),
    Config(Config){};

  RValueEmitter(const RValueEmitter&) = delete;
  RValueEmitter& operator=(const RValueEmitter&) = delete;

  RValueEmitter(RValueEmitter&&) = delete;
  RValueEmitter& operator=(RValueEmitter&&) = delete;

  RValue visitExpr(Expr * E) {
    llvm_unreachable("Unhandled expression found!");
  }

#define W2N_LOG_VISIT()                                                  \
  llvm::outs() << "[RValueEmitter] " << __FUNCTION__ << "\n";

  // Grab a global variable address an push to the stack.
  RValue visitGlobalGetExpr(GlobalGetExpr * E) {
    W2N_LOG_VISIT();
    auto GlobalIter = Fn->getModule()->global_begin();
    std::advance(GlobalIter, E->getGlobalIndex());
    auto& Global = *GlobalIter;
    auto Addr = IGM.getAddrOfGlobalVariable(&Global, NotForDefinition);
    Config.push<Operand>(Addr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitGlobalSetExpr(GlobalSetExpr * E) {
    W2N_LOG_VISIT();
    auto * Op = Config.pop<Operand>();
    auto GlobalIter = Fn->getModule()->global_begin();
    std::advance(GlobalIter, E->getGlobalIndex());
    auto& Global = *GlobalIter;
    auto Addr = IGM.getAddrOfGlobalVariable(&Global, NotForDefinition);
    Builder.CreateStore(Op->getLowered(), Addr);
    Config.push<Operand>(Addr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitLocalSetExpr(LocalSetExpr * E) {
    W2N_LOG_VISIT();
    auto * Op = Config.pop<Operand>();
    auto * F = Config.findTopmost<Frame>();
    assert(F);
    auto Asignee = F->getLocals().at(E->getLocalIndex());
    // FIXME: Alignment
    Alignment FixedAlignment = Alignment(4);
    auto * Load = Builder.CreateLoad(
      Asignee, llvm::Twine("local$") + llvm::Twine(E->getLocalIndex())
    );
    Builder.CreateStore(
      Op->getLowered(), Load->getPointerOperand(), FixedAlignment
    );
    return RValue();
  }

  RValue visitIntegerConstExpr(IntegerConstExpr * E) {
    W2N_LOG_VISIT();
    auto * Ty = IGM.getType(E->getIntegerType());
    auto * ConstVal = llvm::ConstantInt::get(Ty, E->getValue());
    Config.push<Operand>(ConstVal);
    return RValue(Config.top<Operand>());
  }

  RValue visitLocalGetExpr(LocalGetExpr * E) {
    W2N_LOG_VISIT();
    auto * F = Config.findTopmost<Frame>();
    auto LocalAddr = F->getLocals().at(E->getLocalIndex());
    Config.push<Operand>(LocalAddr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitDropExpr(DropExpr * E) {
    W2N_LOG_VISIT();
    Config.pop<Operand>();
    return RValue();
  }

  RValue visitStoreExpr(StoreExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitLoadExpr(LoadExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitCallExpr(CallExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitCallBuiltinExpr(CallBuiltinExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

#undef LOG_VISIT
};

} // namespace

#pragma mark - IRGenFunction

RValue IRGenFunction::emitRValue(Expr * E) {
  return RValueEmitter(Fn, IGM, Builder, *TopConfig).visit(E);
}
