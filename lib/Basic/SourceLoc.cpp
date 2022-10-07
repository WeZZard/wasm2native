#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/SourceManager.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/raw_ostream.h>

using namespace w2n;

void SourceLoc::printLineAndColumn(raw_ostream &OS, const SourceManager &SM,
                                   unsigned BufferID) const {
  if (isInvalid()) {
    OS << "<invalid loc>";
    return;
  }

  auto LineAndCol = SM.getPresumedLineAndColumnForLoc(*this, BufferID);
  OS << "line:" << LineAndCol.first << ':' << LineAndCol.second;
}

void SourceLoc::print(raw_ostream &OS, const SourceManager &SM,
                      unsigned &LastBufferID) const {
  if (isInvalid()) {
    OS << "<invalid loc>";
    return;
  }

  unsigned BufferID = SM.findBufferContainingLoc(*this);
  if (BufferID != LastBufferID) {
    OS << SM.getIdentifierForBuffer(BufferID);
    LastBufferID = BufferID;
  } else {
    OS << "line";
  }

  auto LineAndCol = SM.getPresumedLineAndColumnForLoc(*this, BufferID);
  OS << ':' << LineAndCol.first << ':' << LineAndCol.second;
}

void SourceLoc::dump(const SourceManager &SM) const {
  print(llvm::errs(), SM);
}

CharSourceRange::CharSourceRange(const SourceManager &SM, SourceLoc Start,
                                 SourceLoc End)
    : Start(Start) {
  assert(Start.isValid() == End.isValid() &&
         "Start and end should either both be valid or both be invalid!");
  if (Start.isValid())
    ByteLength = SM.getByteDistance(Start, End);
}

void CharSourceRange::print(raw_ostream &OS, const SourceManager &SM,
                            unsigned &LastBufferID, bool PrintText) const {
  OS << '[';
  Start.print(OS, SM, LastBufferID);
  OS << " - ";
  getEnd().print(OS, SM, LastBufferID);
  OS << ']';

  if (Start.isInvalid() || getEnd().isInvalid())
    return;

  if (PrintText) {
    OS << " RangeText=\"" << SM.extractText(*this) << '"';
  }
}

void CharSourceRange::dump(const SourceManager &SM) const {
  print(llvm::errs(), SM);
}
