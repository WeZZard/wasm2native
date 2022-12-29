//===--- Lexer.h - WebAssembly Language Lexer -------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
//  This file defines the Lexer interface.
//
//===----------------------------------------------------------------===//

#ifndef W2N_PARSE_LEXER_H
#define W2N_PARSE_LEXER_H

#include <memory>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Parse/Token.h>

namespace w2n {

class WasmLexer;
class WatLexer;

class Lexer {
private:

  Lexer(const Lexer&) = delete;
  void operator=(const Lexer&) = delete;

protected:

  const LanguageOptions& LangOpts;

  const SourceManager& SourceMgr;

  unsigned BufferID;

  DiagnosticEngine * Diags;

  /// A queue of diagnostics to emit when a token is consumed. We want to
  /// queue them, as the parser may backtrack and re-lex a token.
  Optional<DiagnosticQueue> DiagQueue;

  Lexer(
    const LanguageOptions& LangOpts,
    const SourceManager& SourceMgr,
    unsigned BufferID,
    DiagnosticEngine * Diags
  );

public:

  static std::unique_ptr<WasmLexer> createWasm(
    const LanguageOptions& LangOpts,
    const SourceManager& SourceMgr,
    unsigned BufferID,
    DiagnosticEngine * Diags
  );

  // TODO: createWat
};

class WasmLexerState {
public:

  WasmLexerState() {
  }

  bool isValid() const {
    return Loc.isValid();
  }

  WasmLexerState advance(unsigned Offset) const {
    assert(isValid());
    return WasmLexerState(Loc.getAdvancedLoc(Offset));
  }

private:

  explicit WasmLexerState(SourceLoc Loc) : Loc(Loc) {
  }

  SourceLoc Loc;
  StringRef LeadingTrivia;
  friend class Lexer;
};

class WasmLexer : public Lexer {
private:

  const char * BufferStart;

  const char * BufferEnd;

  const char * ArtificialEOF = nullptr;

  const char * CurPtr;

  Token NextToken;

  void initialize(unsigned Offset, unsigned EndOffset);

  void lexImpl();

public:

  using State = WasmLexerState;

  WasmLexer(
    const LanguageOptions& LangOpts,
    const SourceManager& SourceMgr,
    unsigned BufferID,
    DiagnosticEngine * Diags
  );

  void lex(Token& Result);

  void resetToOffset(size_t Offset);

  unsigned getBufferID() const;

  const Token& peekNextToken() const;

  State getStateForBeginningOfTokenLoc(SourceLoc Loc) const;

  State getStateForBeginningOfToken(const Token& Tok) const;

  State getStateForEndOfTokenLoc(SourceLoc Loc) const;

  void restoreState(State S, bool EnableDiagnostics = false);

  void backtrackToState(State S);

  SourceLoc getLocForStartOfBuffer() const;
};

class WatLexer : public Lexer {};

} // namespace w2n

#endif // W2N_PARSE_LEXER_H
