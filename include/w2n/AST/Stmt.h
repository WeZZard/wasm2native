#ifndef W2N_AST_STMT_H
#define W2N_AST_STMT_H

#include <_types/_uint32_t.h>
#include "llvm/ADT/Optional.h"
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/Basic/InlineBitfield.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMRTTI.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/Unimplemented.h>

namespace w2n {

enum class StmtKind : uint8_t {
#define STMT(ID, PARENT) ID,
#define LAST_STMT(ID)    Last_Stmt = ID,
#define STMT_RANGE(Id, FirstId, LastId)                                  \
  First_##Id##Stmt = FirstId, Last_##Id##Stmt = LastId,
#include <w2n/AST/StmtNodes.def>
};

enum : unsigned {
  NumStmtKindBits =
    countBitsUsed(static_cast<unsigned>(StmtKind::Last_Stmt))
};

/// Stmt - Base class for all statements in w2n.
class LLVM_POINTER_LIKE_ALIGNMENT(Stmt) Stmt : public ASTAllocated<Stmt> {
  Stmt(const Stmt&) = delete;
  Stmt& operator=(const Stmt&) = delete;

protected:

  StmtKind Kind;

public:

  Stmt(StmtKind Kind) : Kind(Kind) {
  }

  StmtKind getKind() const {
    return Kind;
  }

  /// Retrieve the name of the given statement kind.
  ///
  /// This name should only be used for debugging dumps and other
  /// developer aids, and should never be part of a diagnostic or exposed
  /// to the user of the compiler in any way.
  static StringRef getKindName(StmtKind Kind);

  /// Return the location of the start of the statement.
  SourceLoc getStartLoc() const;

  /// Return the location of the end of the statement.
  SourceLoc getEndLoc() const;

  SourceRange getSourceRange() const {
    w2n_proto_implemented([] { return SourceRange(); });
  }

  /// walk - This recursively walks the AST rooted at this statement.
  // FIXME: Stmt *walk(ASTWalker &walker);
  // FIXME: Stmt *walk(ASTWalker &&walker);

  W2N_DEBUG_DUMP;

  void dump(
    raw_ostream& OS, const ASTContext * Ctx = nullptr, unsigned Indent = 0
  ) const {
    w2n_proto_implemented([] {});
  }
};

class UnreachableStmt : public Stmt {
private:

  UnreachableStmt() : Stmt(StmtKind::Unreachable) {
  }

public:

  static UnreachableStmt * create(ASTContext& Ctx) {
    return new (Ctx) UnreachableStmt();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Unreachable);
};

class BlockStmt : public Stmt {
private:

  BlockType * Ty;

  std::vector<InstNode> Instructions;

  EndStmt * End;

  BlockStmt(
    BlockType * Ty, std::vector<InstNode> Instructions, EndStmt * End
  ) :
    Stmt(StmtKind::Block),
    Ty(Ty),
    Instructions(Instructions),
    End(End) {
  }

public:

  static BlockStmt * create(
    ASTContext& Ctx,
    BlockType * Ty,
    std::vector<InstNode> Instructions,
    EndStmt * End
  ) {
    return new (Ctx) BlockStmt(Ty, Instructions, End);
  }

  BlockType * getType() {
    return Ty;
  }

  const BlockType * getType() const {
    return Ty;
  }

  std::vector<InstNode>& getInstructions() {
    return Instructions;
  }

  const std::vector<InstNode>& getInstructions() const {
    return Instructions;
  }

  EndStmt * getEndStmt() {
    return End;
  }

  const EndStmt * getEndStmt() const {
    return End;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Block);
};

class EndStmt : public Stmt {
private:

  EndStmt() : Stmt(StmtKind::End) {
  }

public:

  static EndStmt * create(ASTContext& Ctx) {
    return new (Ctx) EndStmt();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, End);
};

class LabeledStmt : public Stmt {
protected:

  LabeledStmt(StmtKind Kind) : Stmt(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Stmt, LabeledStmt);
};

class LoopStmt : public LabeledStmt {
private:

  BlockType * Ty;

  std::vector<InstNode> Instructions;

  EndStmt * End;

  LoopStmt(
    BlockType * Ty, std::vector<InstNode> Instructions, EndStmt * End
  ) :
    LabeledStmt(StmtKind::Loop),
    Ty(Ty),
    Instructions(Instructions),
    End(End) {
  }

public:

  static LoopStmt * create(
    ASTContext& Ctx,
    BlockType * Ty,
    std::vector<InstNode> Instructions,
    EndStmt * End
  ) {
    return new (Ctx) LoopStmt(Ty, Instructions, End);
  }

  BlockType * getType() {
    return Ty;
  }

  const BlockType * getType() const {
    return Ty;
  }

  std::vector<InstNode>& getInstructions() {
    return Instructions;
  }

  const std::vector<InstNode>& getInstructions() const {
    return Instructions;
  }

  EndStmt * getEndStmt() {
    return End;
  }

  const EndStmt * getEndStmt() const {
    return End;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Loop);
};

class ElseStmt;

class IfStmt : public LabeledStmt {
private:

  BlockType * Ty;

  std::vector<InstNode> TrueInstructions;

  ElseStmt * Else;

  llvm::Optional<std::vector<InstNode>> FalseInstructions;

  EndStmt * End;

  IfStmt(
    BlockType * Ty,
    std::vector<InstNode> TrueInstructions,
    ElseStmt * Else,
    llvm::Optional<std::vector<InstNode>> FalseInstructions,
    EndStmt * End
  ) :
    LabeledStmt(StmtKind::If),
    Ty(Ty),
    TrueInstructions(TrueInstructions),
    Else(Else),
    FalseInstructions(FalseInstructions),
    End(End) {
  }

public:

  static IfStmt * create(
    ASTContext& Ctx,
    BlockType * Ty,
    std::vector<InstNode> TrueInstructions,
    ElseStmt * Else,
    llvm::Optional<std::vector<InstNode>> FalseInstructions,
    EndStmt * End
  ) {
    return new (Ctx)
      IfStmt(Ty, TrueInstructions, Else, FalseInstructions, End);
  }

  BlockType * getType() {
    return Ty;
  }

  const BlockType * getType() const {
    return Ty;
  }

  std::vector<InstNode>& getTrueInstructions() {
    return TrueInstructions;
  }

  const std::vector<InstNode>& getTrueInstructions() const {
    return TrueInstructions;
  }

  ElseStmt * getElse() {
    return Else;
  }

  const ElseStmt * getElse() const {
    return Else;
  }

  llvm::Optional<std::vector<InstNode>>& getFalseInstructions() {
    return FalseInstructions;
  }

  const llvm::Optional<std::vector<InstNode>>&
  getFalseInstructions() const {
    return FalseInstructions;
  }

  EndStmt * getEnd() {
    return End;
  }

  const EndStmt * getEnd() const {
    return End;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, If);
};

class ElseStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Else);
};

class BrStmt : public LabeledStmt {
private:

  uint32_t LabelIndex;

  BrStmt(uint32_t LabelIndex) :
    LabeledStmt(StmtKind::Br),
    LabelIndex(LabelIndex) {
  }

public:

  static BrStmt * create(ASTContext& Context, uint32_t LabelIndex) {
    return new (Context) BrStmt(LabelIndex);
  }

  uint32_t getLabelIndex() const {
    return LabelIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Br);
};

class BrIfStmt : public LabeledStmt {
private:

  uint32_t LabelIndex;

  BrIfStmt(uint32_t LabelIndex) :
    LabeledStmt(StmtKind::BrIf),
    LabelIndex(LabelIndex) {
  }

public:

  static BrIfStmt * create(ASTContext& Context, uint32_t LabelIndex) {
    return new (Context) BrIfStmt(LabelIndex);
  }

  uint32_t getLabelIndex() const {
    return LabelIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, BrIf);
};

class BrTableStmt : public LabeledStmt {
private:

  std::vector<uint32_t> LabelIndices;

  uint32_t DefaultLabelIndex;

  BrTableStmt(
    std::vector<uint32_t> LabelIndices, uint32_t DefaultLabelIndex
  ) :
    LabeledStmt(StmtKind::BrTable),
    DefaultLabelIndex(DefaultLabelIndex) {
  }

public:

  static BrTableStmt * create(
    ASTContext& Context,
    std::vector<uint32_t> LabelIndices,
    uint32_t DefaultLabelIndex
  ) {
    return new (Context) BrTableStmt(LabelIndices, DefaultLabelIndex);
  }

  std::vector<uint32_t>& getLabelIndices() {
    return LabelIndices;
  }

  const std::vector<uint32_t>& getLabelIndices() const {
    return LabelIndices;
  }

  uint32_t getDefaultLabelIndex() const {
    return DefaultLabelIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, BrTable);
};

class ReturnStmt : public Stmt {
private:

  ReturnStmt() : Stmt(StmtKind::Return) {
  }

public:

  static ReturnStmt * create(ASTContext& Ctx) {
    return new (Ctx) ReturnStmt();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Return);
};

} // namespace w2n

#endif // W2N_AST_STMT_H