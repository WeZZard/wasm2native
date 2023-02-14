#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/DiagnosticSuppression.h>
#include <w2n/AST/DiagnosticsCommon.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/Range.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Localization/LocalizationFormat.h>

using namespace w2n;

namespace {
enum class DiagnosticOptions {
  /// No options.
  None,

  /// The location of this diagnostic points to the beginning of the first
  /// token that the parser considers invalid.  If this token is located
  /// at the
  /// beginning of the line, then the location is adjusted to point to the
  /// end
  /// of the previous token.
  ///
  /// This behavior improves experience for "expected token X"
  /// diagnostics.
  PointsToFirstBadToken,

  /// After a fatal error subsequent diagnostics are suppressed.
  Fatal,

  /// An API or ABI breakage diagnostic emitted by the API digester.
  APIDigesterBreakage,

  /// A deprecation warning or error.
  Deprecation,

  /// A diagnostic warning about an unused element.
  NoUsage,
};

struct StoredDiagnosticInfo {
  DiagnosticKind Kind : 2;
  bool PointsToFirstBadToken : 1;
  bool IsFatal : 1;
  bool IsApiDigesterBreakage : 1;
  bool IsDeprecation : 1;
  bool IsNoUsage : 1;

  constexpr StoredDiagnosticInfo(
    DiagnosticKind K,
    bool FirstBadToken,
    bool Fatal,
    bool IsApiDigesterBreakage,
    bool Deprecation,
    bool NoUsage
  ) :
    Kind(K),
    PointsToFirstBadToken(FirstBadToken),
    IsFatal(Fatal),
    IsApiDigesterBreakage(IsApiDigesterBreakage),
    IsDeprecation(Deprecation),
    IsNoUsage(NoUsage) {
  }

  constexpr StoredDiagnosticInfo(
    DiagnosticKind K, DiagnosticOptions Opts
  ) :
    StoredDiagnosticInfo(
      K,
      Opts == DiagnosticOptions::PointsToFirstBadToken,
      Opts == DiagnosticOptions::Fatal,
      Opts == DiagnosticOptions::APIDigesterBreakage,
      Opts == DiagnosticOptions::Deprecation,
      Opts == DiagnosticOptions::NoUsage
    ) {
  }
};

// Reproduce the DiagIDs, as we want both the size and access to the raw
// ids themselves.
enum LocalDiagID : uint32_t {
#define DIAG(KIND, ID, Options, Text, Signature) ID,
#include <w2n/AST/DiagnosticsAll.def>
  NumDiags
};
} // end anonymous namespace

// TODO: categorization
static const constexpr StoredDiagnosticInfo StoredDiagnosticInfos[] = {
#define ERROR(ID, Options, Text, Signature)                              \
  StoredDiagnosticInfo(DiagnosticKind::Error, DiagnosticOptions::Options),
#define WARNING(ID, Options, Text, Signature)                            \
  StoredDiagnosticInfo(                                                  \
    DiagnosticKind::Warning, DiagnosticOptions::Options                  \
  ),
#define NOTE(ID, Options, Text, Signature)                               \
  StoredDiagnosticInfo(DiagnosticKind::Note, DiagnosticOptions::Options),
#define REMARK(ID, Options, Text, Signature)                             \
  StoredDiagnosticInfo(                                                  \
    DiagnosticKind::Remark, DiagnosticOptions::Options                   \
  ),
#include <w2n/AST/DiagnosticsAll.def>
};
static_assert(
  sizeof(StoredDiagnosticInfos) / sizeof(StoredDiagnosticInfo)
    == LocalDiagID::NumDiags,
  "array size mismatch"
);

static constexpr const char * const DiagnosticStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) Text,
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const DebugDiagnosticStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) Text " [" #ID "]",
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const DiagnosticIdStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) #ID,
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const FixItStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature)
#define FIXIT(ID, Text, Signature) Text,
#include <w2n/AST/DiagnosticsAll.def>
  "<not a fix-it>",
};

#define EDUCATIONAL_NOTES(DIAG, ...)                                     \
  static constexpr const char * const DIAG##_educationalNotes[] = {      \
    __VA_ARGS__, nullptr};
#include <w2n/AST/EducationalNotes.def>

// NOTE: sadly, while GCC and Clang support array designators in C++, they
// are not part of the standard at the moment, so Visual C++ doesn't
// support them. This construct allows us to provide a constexpr array
// initialized to empty values except in the cases that
// EducationalNotes.def are provided, similar to what the C array would
// have looked like.
template <int N>
struct EducationalNotes {
  constexpr EducationalNotes() : Value() {
    for (auto I = 0; I < N; ++I) {
      Value[I] = {};
    }
#define EDUCATIONAL_NOTES(DIAG, ...)                                     \
  value[LocalDiagID::DIAG] = DIAG##_educationalNotes;
#include <w2n/AST/EducationalNotes.def>
  }

  const char * const * Value[N];
};

static constexpr EducationalNotes<LocalDiagID::NumDiags>
  AllEducationalNotesStore = EducationalNotes<LocalDiagID::NumDiags>();
static constexpr auto AllEducationalNotes =
  AllEducationalNotesStore.Value;

DiagnosticState::DiagnosticState() {
  // Initialize our ignored diagnostics to default
  IgnoredDiagnostics.resize(LocalDiagID::NumDiags);
}

static CharSourceRange
toCharSourceRange(SourceManager& SM, SourceRange SR) {
  // FIXME: return CharSourceRange(SM, SR.Start,
  // Lexer::getLocForEndOfToken(SM, SR.End));
  w2n_unimplemented();
}

static CharSourceRange
toCharSourceRange(SourceManager& SM, SourceLoc Start, SourceLoc End) {
  return CharSourceRange(SM, Start, End);
}

/// Extract a character at \p Loc. If \p Loc is the end of the buffer,
/// return '\f'.
static char extractCharAfter(SourceManager& SM, SourceLoc Loc) {
  auto Chars = SM.extractText({Loc, 1});
  return Chars.empty() ? '\f' : Chars[0];
}

/// Extract a character immediately before \p Loc. If \p Loc is the
/// start of the buffer, return '\f'.
static char extractCharBefore(SourceManager& SM, SourceLoc Loc) {
  // We have to be careful not to go off the front of the buffer.
  auto BufferId = SM.findBufferContainingLoc(Loc);
  auto BufferRange = SM.getRangeForBuffer(BufferId);
  if (BufferRange.getStart() == Loc) {
    return '\f';
  }
  auto Chars = SM.extractText({Loc.getAdvancedLoc(-1), 1}, BufferId);
  assert(!Chars.empty() && "Couldn't extractText with valid range");
  return Chars[0];
}

InFlightDiagnostic& InFlightDiagnostic::highlight(SourceRange R) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if ((Engine != nullptr) && R.isValid()) {
    Engine->getActiveDiagnostic().addRange(
      toCharSourceRange(Engine->SourceMgr, R)
    );
  }
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::highlightChars(SourceLoc Start, SourceLoc End) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if ((Engine != nullptr) && Start.isValid()) {
    Engine->getActiveDiagnostic().addRange(
      toCharSourceRange(Engine->SourceMgr, Start, End)
    );
  }
  return *this;
}

/// Add an insertion fix-it to the currently-active diagnostic.  The
/// text is inserted immediately *after* the token specified.
///
InFlightDiagnostic& InFlightDiagnostic::fixItInsertAfter(
  SourceLoc L, StringRef FormatString, ArrayRef<DiagnosticArgument> Args
) {
  // L = Lexer::getLocForEndOfToken(Engine->SourceMgr, L);
  // return fixItInsert(L, FormatString, Args);
  w2n_unimplemented();
}

/// Add a token-based removal fix-it to the currently-active
/// diagnostic.
InFlightDiagnostic& InFlightDiagnostic::fixItRemove(SourceRange R) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (R.isInvalid() || (Engine == nullptr)) {
    return *this;
  }

  // Convert from a token range to a CharSourceRange, which points to the
  // end of the token we want to remove.
  auto& SM = Engine->SourceMgr;
  auto CharRange = toCharSourceRange(SM, R);

  // If we're removing something (e.g. a keyword), do a bit of extra work
  // to make sure that we leave the code in a good place, without
  // extraneous white space around its hole.  Specifically, check to see
  // there is whitespace before and after the end of range.  If so, nuke
  // the space afterward to keep things consistent.
  if (
    extractCharAfter(SM, CharRange.getEnd()) == ' ' &&
    (isspace(extractCharBefore(SM, CharRange.getStart())) != 0)) {
    CharRange = CharSourceRange(
      CharRange.getStart(), CharRange.getByteLength() + 1
    );
  }
  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(CharRange, {}, {})
  );
  return *this;
}

InFlightDiagnostic& InFlightDiagnostic::fixItReplace(
  SourceRange R, StringRef FormatString, ArrayRef<DiagnosticArgument> Args
) {
  auto& SM = Engine->SourceMgr;
  auto CharRange = toCharSourceRange(SM, R);

  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(CharRange, FormatString, Args)
  );
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::fixItReplace(SourceRange R, StringRef Str) {
  if (Str.empty()) {
    return fixItRemove(R);
  }

  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (R.isInvalid() || (Engine == nullptr)) {
    return *this;
  }

  auto& SM = Engine->SourceMgr;
  auto CharRange = toCharSourceRange(SM, R);

  // If we're replacing with something that wants spaces around it, do a
  // bit of extra work so that we don't suggest extra spaces.
  // FIXME: This could probably be applied to structured fix-its as well.
  if (Str.back() == ' ') {
    if (isspace(extractCharAfter(SM, CharRange.getEnd())) != 0) {
      Str = Str.drop_back();
    }
  }
  if (!Str.empty() && Str.front() == ' ') {
    if (isspace(extractCharBefore(SM, CharRange.getStart())) != 0) {
      Str = Str.drop_front();
    }
  }

  return fixItReplace(R, "%0", {Str});
}

InFlightDiagnostic& InFlightDiagnostic::fixItReplaceChars(
  SourceLoc Start,
  SourceLoc End,
  StringRef FormatString,
  ArrayRef<DiagnosticArgument> Args
) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if ((Engine != nullptr) && Start.isValid()) {
    Engine->getActiveDiagnostic().addFixIt(Diagnostic::FixIt(
      toCharSourceRange(Engine->SourceMgr, Start, End), FormatString, Args
    ));
  }
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::fixItExchange(SourceRange R1, SourceRange R2) {
  assert(IsActive && "Cannot modify an inactive diagnostic");

  auto& SM = Engine->SourceMgr;
  // Convert from a token range to a CharSourceRange
  auto CharRange1 = toCharSourceRange(SM, R1);
  auto CharRange2 = toCharSourceRange(SM, R2);
  // Extract source text.
  auto Text1 = SM.extractText(CharRange1);
  auto Text2 = SM.extractText(CharRange2);

  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(CharRange1, "%0", {Text2})
  );
  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(CharRange2, "%0", {Text1})
  );
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::limitBehavior(DiagnosticBehavior Limit) {
  Engine->getActiveDiagnostic().setBehaviorLimit(Limit);
  return *this;
}

InFlightDiagnostic& InFlightDiagnostic::wrapIn(const Diagnostic& Wrapper
) {
  // Save current active diagnostic into WrappedDiagnostics, ignoring
  // state so we don't get a None return or influence future diagnostics.
  DiagnosticState TempState;
  Engine->State.swap(TempState);
  llvm::SaveAndRestore<DiagnosticBehavior> Limit(
    Engine->getActiveDiagnostic().BehaviorLimit,
    DiagnosticBehavior::Unspecified
  );

  Engine->WrappedDiagnostics.push_back(
    *Engine->diagnosticInfoForDiagnostic(Engine->getActiveDiagnostic())
  );

  Engine->State.swap(TempState);

  auto& Wrapped = Engine->WrappedDiagnostics.back();

  // Copy and update its arg list.
  Engine->WrappedDiagnosticArgs.emplace_back(Wrapped.FormatArgs);
  Wrapped.FormatArgs = Engine->WrappedDiagnosticArgs.back();

  // Overwrite the ID and argument with those from the wrapper.
  Engine->getActiveDiagnostic().ID = Wrapper.ID;
  Engine->getActiveDiagnostic().Args = Wrapper.Args;

  // Set the argument to the diagnostic being wrapped.
  assert(
    Wrapper.getArgs().front().getKind()
    == DiagnosticArgumentKind::Diagnostic
  );
  Engine->getActiveDiagnostic().Args.front() = &Wrapped;

  return *this;
}

void InFlightDiagnostic::flush() {
  if (!IsActive) {
    return;
  }

  IsActive = false;
  if (Engine != nullptr) {
    Engine->flushActiveDiagnostic();
  }
}

void Diagnostic::addChildNote(Diagnostic&& D) {
  assert(
    StoredDiagnosticInfos[(unsigned)D.ID].Kind == DiagnosticKind::Note
    && "Only notes can have a parent."
  );
  assert(
    StoredDiagnosticInfos[(unsigned)ID].Kind != DiagnosticKind::Note
    && "Notes can't have children."
  );
  ChildNotes.push_back(std::move(D));
}

bool DiagnosticEngine::isDiagnosticPointsToFirstBadToken(DiagID ID
) const {
  return StoredDiagnosticInfos[(unsigned)ID].PointsToFirstBadToken;
}

bool DiagnosticEngine::isAPIDigesterBreakageDiagnostic(DiagID ID) const {
  return StoredDiagnosticInfos[(unsigned)ID].IsApiDigesterBreakage;
}

bool DiagnosticEngine::isDeprecationDiagnostic(DiagID ID) const {
  return StoredDiagnosticInfos[(unsigned)ID].IsDeprecation;
}

bool DiagnosticEngine::isNoUsageDiagnostic(DiagID ID) const {
  return StoredDiagnosticInfos[(unsigned)ID].IsNoUsage;
}

bool DiagnosticEngine::finishProcessing() {
  bool HadError = false;
  for (auto& Consumer : Consumers) {
    HadError |= Consumer->finishProcessing();
  }
  return HadError;
}

/// Skip forward to one of the given delimiters.
///
/// \param Text The text to search through, which will be updated to point
/// just after the delimiter.
///
/// \param Delim The first character delimiter to search for.
///
/// \param FoundDelim On return, true if the delimiter was found, or false
/// if the end of the string was reached.
///
/// \returns The string leading up to the delimiter, or the empty string
/// if no delimiter is found.
static StringRef skipToDelimiter(
  StringRef& Text, char Delim, bool * FoundDelim = nullptr
) {
  unsigned Depth = 0;
  if (FoundDelim != nullptr) {
    *FoundDelim = false;
  }

  unsigned I = 0;
  for (unsigned N = Text.size(); I != N; ++I) {
    if (Text[I] == '{') {
      ++Depth;
      continue;
    }
    if (Depth > 0) {
      if (Text[I] == '}') {
        --Depth;
      }
      continue;
    }

    if (Text[I] == Delim) {
      if (FoundDelim != nullptr) {
        *FoundDelim = true;
      }
      break;
    }
  }

  assert(Depth == 0 && "Unbalanced {} set in diagnostic text");
  StringRef Result = Text.substr(0, I);
  Text = Text.substr(I + 1);
  return Result;
}

/// Handle the integer 'select' modifier.  This is used like this:
/// %select{foo|bar|baz}2.  This means that the integer argument "%2" has
/// a value from 0-2.  If the value is 0, the diagnostic prints 'foo'. If
/// the value is 1, it prints 'bar'.  If it has the value 2, it prints
/// 'baz'. This is very useful for certain classes of variant diagnostics.
static void formatSelectionArgument(
  StringRef ModifierArguments,
  ArrayRef<DiagnosticArgument> Args,
  unsigned SelectedIndex,
  DiagnosticFormatOptions FormatOpts,
  llvm::raw_ostream& os
) {
  bool FoundPipe = false;
  do {
    assert(
      (!ModifierArguments.empty() || FoundPipe)
      && "Index beyond bounds in %select modifier"
    );
    StringRef Text = skipToDelimiter(ModifierArguments, '|', &FoundPipe);
    if (SelectedIndex == 0) {
      DiagnosticEngine::formatDiagnosticText(os, Text, Args, FormatOpts);
      break;
    }
    --SelectedIndex;
  } while (true);
}

/// Format a single diagnostic argument and write it to the given
/// stream.
static void formatDiagnosticArgument(
  StringRef Modifier,
  StringRef ModifierArguments,
  ArrayRef<DiagnosticArgument> Args,
  unsigned ArgIndex,
  DiagnosticFormatOptions FormatOpts,
  llvm::raw_ostream& os
) {
  const DiagnosticArgument& Arg = Args[ArgIndex];
  switch (Arg.getKind()) {
  case DiagnosticArgumentKind::Integer:
    if (Modifier == "select") {
      assert(Arg.getAsInteger() >= 0 && "Negative selection index");
      formatSelectionArgument(
        ModifierArguments, Args, Arg.getAsInteger(), FormatOpts, os
      );
    } else if (Modifier == "s") {
      if (Arg.getAsInteger() != 1) {
        os << 's';
      }
    } else {
      assert(
        Modifier.empty() && "Improper modifier for integer argument"
      );
      os << Arg.getAsInteger();
    }
    break;

  case DiagnosticArgumentKind::Unsigned:
    if (Modifier == "select") {
      formatSelectionArgument(
        ModifierArguments, Args, Arg.getAsUnsigned(), FormatOpts, os
      );
    } else if (Modifier == "s") {
      if (Arg.getAsUnsigned() != 1) {
        os << 's';
      }
    } else {
      assert(
        Modifier.empty() && "Improper modifier for unsigned argument"
      );
      os << Arg.getAsUnsigned();
    }
    break;

  case DiagnosticArgumentKind::String:
    if (Modifier == "select") {
      formatSelectionArgument(
        ModifierArguments,
        Args,
        Arg.getAsString().empty() ? 0 : 1,
        FormatOpts,
        os
      );
    } else {
      assert(Modifier.empty() && "Improper modifier for string argument");
      os << Arg.getAsString();
    }
    break;

  case DiagnosticArgumentKind::Diagnostic: {
    assert(
      Modifier.empty() && "Improper modifier for Diagnostic argument"
    );
    auto * DiagArg = Arg.getAsDiagnostic();
    DiagnosticEngine::formatDiagnosticText(
      os, DiagArg->FormatString, DiagArg->FormatArgs
    );
    break;
  }
  }
}

/// Format the given diagnostic text and place the result in the given
/// buffer.
void DiagnosticEngine::formatDiagnosticText(
  llvm::raw_ostream& os,
  StringRef InText,
  ArrayRef<DiagnosticArgument> Args,
  DiagnosticFormatOptions FormatOpts
) {
  while (!InText.empty()) {
    size_t Percent = InText.find('%');
    if (Percent == StringRef::npos) {
      // Write the rest of the string; we're done.
      os.write(InText.data(), InText.size());
      break;
    }

    // Write the string up to (but not including) the %, then drop that
    // text (including the %).
    os.write(InText.data(), Percent);
    InText = InText.substr(Percent + 1);

    // '%%' -> '%'.
    if (InText[0] == '%') {
      os.write('%');
      InText = InText.substr(1);
      continue;
    }

    // Parse an optional modifier.
    StringRef Modifier;
    {
      size_t Length = InText.find_if_not(isalpha);
      Modifier = InText.substr(0, Length);
      InText = InText.substr(Length);
    }

    if (Modifier == "error") {
      os << StringRef(
        "<<INTERNAL ERROR: encountered %error in diagnostic text>>"
      );
      continue;
    }

    // Parse the optional argument list for a modifier, which is
    // brace-enclosed.
    StringRef ModifierArguments;
    if (InText[0] == '{') {
      InText = InText.substr(1);
      ModifierArguments = skipToDelimiter(InText, '}');
    }

    // Find the digit sequence, and parse it into an argument index.
    size_t Length = InText.find_if_not(isdigit);
    unsigned ArgIndex;
#define W2N_INDEX_RADIX 10
    bool IndexParseFailed =
      InText.substr(0, Length).getAsInteger(W2N_INDEX_RADIX, ArgIndex);
#undef W2N_INDEX_RADIX

    if (IndexParseFailed) {
      os << StringRef("<<INTERNAL ERROR: unparseable argument index in "
                      "diagnostic text>>");
      continue;
    }

    InText = InText.substr(Length);

    if (ArgIndex >= Args.size()) {
      os << StringRef("<<INTERNAL ERROR: out-of-range argument index in "
                      "diagnostic text>>");
      continue;
    }

    // Convert the argument to a string.
    formatDiagnosticArgument(
      Modifier, ModifierArguments, Args, ArgIndex, FormatOpts, os
    );
  }
}

static DiagnosticKind toDiagnosticKind(DiagnosticBehavior Behavior) {
  switch (Behavior) {
  case DiagnosticBehavior::Unspecified:
    llvm_unreachable("unspecified behavior");
  case DiagnosticBehavior::Ignore:
    llvm_unreachable("trying to map an ignored diagnostic");
  case DiagnosticBehavior::Error:
  case DiagnosticBehavior::Fatal: return DiagnosticKind::Error;
  case DiagnosticBehavior::Note: return DiagnosticKind::Note;
  case DiagnosticBehavior::Warning: return DiagnosticKind::Warning;
  case DiagnosticBehavior::Remark: return DiagnosticKind::Remark;
  }

  llvm_unreachable("Unhandled DiagnosticKind in switch.");
}

static DiagnosticBehavior
toDiagnosticBehavior(DiagnosticKind Kind, bool IsFatal) {
  switch (Kind) {
  case DiagnosticKind::Note: return DiagnosticBehavior::Note;
  case DiagnosticKind::Error:
    return IsFatal ? DiagnosticBehavior::Fatal
                   : DiagnosticBehavior::Error;
  case DiagnosticKind::Warning: return DiagnosticBehavior::Warning;
  case DiagnosticKind::Remark: return DiagnosticBehavior::Remark;
  }
  llvm_unreachable("Unhandled DiagnosticKind in switch.");
}

// A special option only for compiler writers that causes Diagnostics to
// assert when a failure diagnostic is emitted. Intended for use in the
// debugger.
llvm::cl::opt<bool>
  AssertOnError("w2n-diagnostics-assert-on-error", llvm::cl::init(false));
// A special option only for compiler writers that causes Diagnostics to
// assert when a warning diagnostic is emitted. Intended for use in the
// debugger.
llvm::cl::opt<bool> AssertOnWarning(
  "w2n-diagnostics-assert-on-warning", llvm::cl::init(false)
);

DiagnosticBehavior
DiagnosticState::determineBehavior(const Diagnostic& Diag) {
  // We determine how to handle a diagnostic based on the following rules
  //   1) Map the diagnostic to its "intended" behavior, applying the
  //   behavior
  //      limit for this particular emission
  //   2) If current state dictates a certain behavior, follow that
  //   3) If the user ignored this specific diagnostic, follow that
  //   4) If the user substituted a different behavior for this behavior,
  //   apply
  //      that change
  //   5) Update current state for use during the next diagnostic

  //   1) Map the diagnostic to its "intended" behavior, applying the
  //   behavior
  //      limit for this particular emission
  auto DiagInfo = StoredDiagnosticInfos[(unsigned)Diag.getID()];
  DiagnosticBehavior Lvl = std::max(
    toDiagnosticBehavior(DiagInfo.Kind, DiagInfo.IsFatal),
    Diag.getBehaviorLimit()
  );
  assert(Lvl != DiagnosticBehavior::Unspecified);

  //   2) If current state dictates a certain behavior, follow that

  // Notes relating to ignored diagnostics should also be ignored
  if (PreviousBehavior == DiagnosticBehavior::Ignore && Lvl == DiagnosticBehavior::Note) {
    Lvl = DiagnosticBehavior::Ignore;
  }

  // Suppress diagnostics when in a fatal state, except for follow-on
  // notes
  if (FatalErrorOccurred) {
    if (!ShowDiagnosticsAfterFatalError && Lvl != DiagnosticBehavior::Note) {
      Lvl = DiagnosticBehavior::Ignore;
    }
  }

  //   3) If the user ignored this specific diagnostic, follow that
  if (IgnoredDiagnostics[(unsigned)Diag.getID()]) {
    Lvl = DiagnosticBehavior::Ignore;
  }

  //   4) If the user substituted a different behavior for this behavior,
  //   apply
  //      that change
  if (Lvl == DiagnosticBehavior::Warning) {
    if (WarningsAsErrors) {
      Lvl = DiagnosticBehavior::Error;
    }
    if (SuppressWarnings) {
      Lvl = DiagnosticBehavior::Ignore;
    }
  }

  //   5) Update current state for use during the next diagnostic
  if (Lvl == DiagnosticBehavior::Fatal) {
    FatalErrorOccurred = true;
    AnyErrorOccurred = true;
  } else if (Lvl == DiagnosticBehavior::Error) {
    AnyErrorOccurred = true;
  }

  assert(
    (!AssertOnError || !AnyErrorOccurred) && "We emitted an error?!"
  );
  assert(
    (!AssertOnWarning || (Lvl != DiagnosticBehavior::Warning))
    && "We emitted a warning?!"
  );

  PreviousBehavior = Lvl;
  return Lvl;
}

void DiagnosticEngine::flushActiveDiagnostic() {
  assert(ActiveDiagnostic && "No active diagnostic to flush");
  handleDiagnostic(std::move(*ActiveDiagnostic));
  ActiveDiagnostic.reset();
}

void DiagnosticEngine::handleDiagnostic(Diagnostic&& Diag) {
  if (TransactionCount == 0) {
    emitDiagnostic(Diag);
    WrappedDiagnostics.clear();
    WrappedDiagnosticArgs.clear();
  } else {
    onTentativeDiagnosticFlush(Diag);
    TentativeDiagnostics.emplace_back(std::move(Diag));
  }
}

void DiagnosticEngine::clearTentativeDiagnostics() {
  TentativeDiagnostics.clear();
  WrappedDiagnostics.clear();
  WrappedDiagnosticArgs.clear();
}

void DiagnosticEngine::emitTentativeDiagnostics() {
  for (auto& Diag : TentativeDiagnostics) {
    emitDiagnostic(Diag);
  }
  clearTentativeDiagnostics();
}

void DiagnosticEngine::forwardTentativeDiagnosticsTo(
  DiagnosticEngine& TargetEngine
) {
  for (auto& Diag : TentativeDiagnostics) {
    TargetEngine.handleDiagnostic(std::move(Diag));
  }
  clearTentativeDiagnostics();
}

Optional<DiagnosticInfo>
DiagnosticEngine::diagnosticInfoForDiagnostic(const Diagnostic& Diagnostic
) {
  auto Behavior = State.determineBehavior(Diagnostic);
  if (Behavior == DiagnosticBehavior::Ignore) {
    return None;
  }

  // Figure out the source location.
  SourceLoc Loc = Diagnostic.getLoc();
  if (Loc.isInvalid() && (Diagnostic.getDecl() != nullptr)) {
    const Decl * D = Diagnostic.getDecl();
    // If a declaration was provided instead of a location, and that
    // declaration has a location we can point to, use that location.
    Loc = D->getLoc();

    if (Loc.isInvalid()) {
      // There is no location we can point to. Pretty-print the
      // declaration so we can point to it.
      llvm_unreachable(
        "pretty-print for WebAseembly file is not implemented."
      );
    }
  }

  StringRef Category;
  if (isAPIDigesterBreakageDiagnostic(Diagnostic.getID())) {
    Category = "api-digester-breaking-change";
  } else if (isDeprecationDiagnostic(Diagnostic.getID())) {
    Category = "deprecation";
  } else if (isNoUsageDiagnostic(Diagnostic.getID())) {
    Category = "no-usage";
  }

  return DiagnosticInfo(
    Diagnostic.getID(),
    Loc,
    toDiagnosticKind(Behavior),
    diagnosticStringFor(Diagnostic.getID(), getPrintDiagnosticNames()),
    Diagnostic.getArgs(),
    Category,
    getDefaultDiagnosticLoc(),
    /*child note info*/ {},
    Diagnostic.getRanges(),
    Diagnostic.getFixIts(),
    Diagnostic.isChildNote()
  );
}

void DiagnosticEngine::emitDiagnostic(const Diagnostic& Diag) {
  if (auto Info = diagnosticInfoForDiagnostic(Diag)) {
    SmallVector<DiagnosticInfo, 1> ChildInfo;
    auto ChildNotes = Diag.getChildNotes();
    for (unsigned I : indices(ChildNotes)) {
      auto Child = diagnosticInfoForDiagnostic(ChildNotes[I]);
      assert(Child);
      assert(
        Child->Kind == DiagnosticKind::Note
        && "Expected child diagnostics to all be notes?!"
      );
      ChildInfo.push_back(*Child);
    }
    TinyPtrVector<DiagnosticInfo *> ChildInfoPtrs;
    for (unsigned I : indices(ChildInfo)) {
      ChildInfoPtrs.push_back(&ChildInfo[I]);
    }
    Info->ChildDiagnosticInfo = ChildInfoPtrs;

    SmallVector<std::string, 1> EducationalNotePaths;
    const auto * AssociatedNotes =
      AllEducationalNotes[(uint32_t)Diag.getID()];
    while ((AssociatedNotes != nullptr) && (*AssociatedNotes != nullptr)
    ) {
#define W2N_NOTE_PAHT_LENGTH 128
      SmallString<W2N_NOTE_PAHT_LENGTH> NotePath(
        getDiagnosticDocumentationPath()
      );
      llvm::sys::path::append(NotePath, *AssociatedNotes);
      EducationalNotePaths.push_back(NotePath.str().str());
      ++AssociatedNotes;
#undef W2N_NOTE_PAHT_LENGTH
    }
    Info->EducationalNotePaths = EducationalNotePaths;

    for (auto& Consumer : Consumers) {
      Consumer->handleDiagnostic(SourceMgr, *Info);
    }
  }

  // For compatibility with DiagnosticConsumers which don't know about
  // child notes. These can be ignored by consumers which do take
  // advantage of the grouping.
  for (const auto& ChildNote : Diag.getChildNotes()) {
    emitDiagnostic(ChildNote);
  }
}

DiagnosticKind DiagnosticEngine::declaredDiagnosticKindFor(const DiagID Id
) {
  return StoredDiagnosticInfos[(unsigned)Id].Kind;
}

llvm::StringRef DiagnosticEngine::diagnosticStringFor(
  const DiagID Id, bool PrintDiagnosticNames
) {
  const auto * DefaultMessage = PrintDiagnosticNames
                                ? DebugDiagnosticStrings[(unsigned)Id]
                                : DiagnosticStrings[(unsigned)Id];

  if (auto * Producer = Localization.get()) {
    auto LocalizedMessage = Producer->getMessageOr(Id, DefaultMessage);
    return LocalizedMessage;
  }
  return DefaultMessage;
}

llvm::StringRef DiagnosticEngine::diagnosticIDStringFor(const DiagID Id) {
  return DiagnosticIdStrings[(unsigned)Id];
}

const char * InFlightDiagnostic::fixItStringFor(const FixItID Id) {
  return FixItStrings[(unsigned)Id];
}

void DiagnosticEngine::setBufferIndirectlyCausingDiagnosticToInput(
  SourceLoc Loc
) {
  // If in the future, nested BufferIndirectlyCausingDiagnosticRAII need
  // be supported, the compiler will need a stack for
  // bufferIndirectlyCausingDiagnostic.
  assert(
    BufferIndirectlyCausingDiagnostic.isInvalid()
    && "Buffer should not already be set."
  );
  BufferIndirectlyCausingDiagnostic = Loc;
  assert(
    BufferIndirectlyCausingDiagnostic.isValid()
    && "Buffer must be valid for previous assertion to work."
  );
}

void DiagnosticEngine::resetBufferIndirectlyCausingDiagnostic() {
  BufferIndirectlyCausingDiagnostic = SourceLoc();
}

DiagnosticSuppression::DiagnosticSuppression(DiagnosticEngine& Diags) :
  Diags(Diags) {
  Consumers = Diags.takeConsumers();
}

DiagnosticSuppression::~DiagnosticSuppression() {
  for (auto * Consumer : Consumers) {
    Diags.addConsumer(*Consumer);
  }
}

bool DiagnosticSuppression::isEnabled(const DiagnosticEngine& Diags) {
  return Diags.getConsumers().empty();
}

BufferIndirectlyCausingDiagnosticRAII::
  BufferIndirectlyCausingDiagnosticRAII(const SourceFile& SF) :
  Diags(SF.getASTContext().Diags) {
  auto Id = SF.getBufferID();
  if (!Id) {
    return;
  }
  auto Loc = SF.getASTContext().SourceMgr.getLocForBufferStart(*Id);
  if (Loc.isValid()) {
    Diags.setBufferIndirectlyCausingDiagnosticToInput(Loc);
  }
}

void DiagnosticEngine::onTentativeDiagnosticFlush(Diagnostic& Diagnostic
) {
  for (auto& Argument : Diagnostic.Args) {
    if (Argument.getKind() != DiagnosticArgumentKind::String) {
      continue;
    }

    auto Content = Argument.getAsString();
    if (Content.empty()) {
      continue;
    }

    auto I = TransactionStrings.insert(Content).first;
    Argument = DiagnosticArgument(StringRef(I->getKeyData()));
  }
}
