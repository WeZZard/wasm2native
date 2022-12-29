#ifndef W2N_PARSE_TOKEN_H
#define W2N_PARSE_TOKEN_H

#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Parse/TokenKinds.h>

namespace w2n {

class Token {
private:

  /// Kind - The actual flavor of token this is.
  ///
  TokenKind Kind;

  /// Text - The actual string covered by the token in the source buffer.
  StringRef Text;

public:

  Token(TokenKind Kind, StringRef Text) : Kind(Kind), Text(Text) {
  }

  Token() : Token(TokenKind::NUM_TOKENS, {}) {
  }

  TokenKind getKind() const {
    return Kind;
  }

  void setKind(TokenKind K) {
    Kind = K;
  }

  /// is/isNot - Predicates to check if this token is a specific kind, as
  /// in "if (Tok.is(tok::l_brace)) {...}".
  bool is(TokenKind K) const {
    return Kind == K;
  }

  bool isNot(TokenKind K) const {
    return Kind != K;
  }

  // Predicates to check to see if the token is any of a list of tokens.

  bool isAny(TokenKind K1) const {
    return is(K1);
  }

  template <typename... T>
  bool isAny(TokenKind K1, TokenKind K2, T... K) const {
    if (is(K1)) {
      return true;
    }
    return isAny(K2, K...);
  }

  // Predicates to check to see if the token is not the same as any of a
  // list.
  template <typename... T>
  bool isNot(TokenKind K1, T... K) const {
    return !isAny(K1, K...);
  }

  /// getLoc - Return a source location identifier for the specified
  /// offset in the current file.
  SourceLoc getLoc() const {
    return SourceLoc(llvm::SMLoc::getFromPointer(Text.begin()));
  }

  unsigned getLength() const {
    return Text.size();
  }

  CharSourceRange getRange() const {
    return CharSourceRange(getLoc(), getLength());
  }

  StringRef getText() const {
    // FIXME: Need to consider escape for wat file?
    return Text;
  }

  void setText(StringRef T) {
    Text = T;
  }

  /// Set the token to the specified kind and source range.
  void setToken(TokenKind K, StringRef T) {
    Kind = K;
    Text = T;
  }
};

} // namespace w2n

#endif // W2N_PARSE_TOKEN_H