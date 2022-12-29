#include <memory>
#include <w2n/Parse/Lexer.h>

using namespace w2n;

#pragma mark - Lexer

Lexer::Lexer(
  const LanguageOptions& LangOpts,
  const SourceManager& SourceMgr,
  unsigned BufferID,
  DiagnosticEngine * Diags
) :
  LangOpts(LangOpts),
  SourceMgr(SourceMgr),
  BufferID(BufferID),
  Diags(Diags) {
  if (Diags != nullptr) {
    DiagQueue.emplace(*Diags, /*emitOnDestruction*/ false);
  }
}

std::unique_ptr<WasmLexer> Lexer::createWasm(
  const LanguageOptions& LangOpts,
  const SourceManager& SourceMgr,
  unsigned BufferID,
  DiagnosticEngine * Diags
) {
  return std::make_unique<WasmLexer>(
    LangOpts, SourceMgr, BufferID, Diags
  );
}

#pragma mark - WasmLexer

void WasmLexer::lexImpl() {
}

void WasmLexer::initialize(unsigned Offset, unsigned EndOffset) {
  // Initialize buffer pointers.
  StringRef Contents =
    SourceMgr.extractText(SourceMgr.getRangeForBuffer(BufferID));
  BufferStart = Contents.data();
  BufferEnd = Contents.data() + Contents.size();
  assert(*BufferEnd == 0);
  assert(BufferStart + Offset <= BufferEnd);
  assert(BufferStart + EndOffset <= BufferEnd);

  ArtificialEOF = BufferStart + EndOffset;
  CurPtr = BufferStart + Offset;

  assert(NextToken.is(TokenKind::NUM_TOKENS));
  lexImpl();
}

WasmLexer::WasmLexer(
  const LanguageOptions& LangOpts,
  const SourceManager& SourceMgr,
  unsigned BufferID,
  DiagnosticEngine * Diags
) :
  Lexer(LangOpts, SourceMgr, BufferID, Diags) {
  unsigned EndOffset =
    SourceMgr.getRangeForBuffer(BufferID).getByteLength();
  initialize(/*Offset=*/0, EndOffset);
}
