#ifndef W2N_AST_STMT_H
#define W2N_AST_STMT_H

#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/Basic/InlineBitfield.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMRTTI.h>
#include <w2n/Basic/SourceLoc.h>

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

  SourceRange getSourceRange() const;

  /// walk - This recursively walks the AST rooted at this statement.
  // FIXME: Stmt *walk(ASTWalker &walker);
  // FIXME: Stmt *walk(ASTWalker &&walker);

  W2N_DEBUG_DUMP;
  void dump(
    raw_ostream& OS, const ASTContext * Ctx = nullptr, unsigned Indent = 0
  ) const;
};

class UnreachableStmt : public Stmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Unreachable);
};

class BlockStmt : public Stmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Block);
};

class EndStmt : public Stmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, End);
};

class LabeledStmt : public Stmt {
public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Stmt, LabeledStmt);
};

class LoopStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Loop);
};

class IfStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, If);
};

class BrStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Br);
};

class BrIfStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, BrIf);
};

class BrTableStmt : public LabeledStmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, BrTable);
};

class ReturnStmt : public Stmt {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Stmt, Return);
};

} // namespace w2n

#endif // W2N_AST_STMT_H