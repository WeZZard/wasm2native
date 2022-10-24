#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/DiagnosticsCommon.h>
#include <w2n/AST/DiagnosticSuppression.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/Range.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Localization/LocalizationFormat.h>

using namespace w2n;

namespace {
enum class DiagnosticOptions {
  /// No options.
  none,

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
  DiagnosticKind kind : 2;
  bool pointsToFirstBadToken : 1;
  bool isFatal : 1;
  bool isAPIDigesterBreakage : 1;
  bool isDeprecation : 1;
  bool isNoUsage : 1;

  constexpr StoredDiagnosticInfo(
    DiagnosticKind k,
    bool firstBadToken,
    bool fatal,
    bool isAPIDigesterBreakage,
    bool deprecation,
    bool noUsage
  )
    : kind(k), pointsToFirstBadToken(firstBadToken), isFatal(fatal),
      isAPIDigesterBreakage(isAPIDigesterBreakage),
      isDeprecation(deprecation), isNoUsage(noUsage) {}

  constexpr StoredDiagnosticInfo(DiagnosticKind k, DiagnosticOptions opts)
    : StoredDiagnosticInfo(
        k,
        opts == DiagnosticOptions::PointsToFirstBadToken,
        opts == DiagnosticOptions::Fatal,
        opts == DiagnosticOptions::APIDigesterBreakage,
        opts == DiagnosticOptions::Deprecation,
        opts == DiagnosticOptions::NoUsage
      ) {}
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
static const constexpr StoredDiagnosticInfo storedDiagnosticInfos[] = {
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
  sizeof(storedDiagnosticInfos) / sizeof(StoredDiagnosticInfo) ==
    LocalDiagID::NumDiags,
  "array size mismatch"
);

static constexpr const char * const diagnosticStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) Text,
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const debugDiagnosticStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) Text " [" #ID "]",
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const diagnosticIDStrings[] = {
#define DIAG(KIND, ID, Options, Text, Signature) #ID,
#include <w2n/AST/DiagnosticsAll.def>
  "<not a diagnostic>",
};

static constexpr const char * const fixItStrings[] = {
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
  constexpr EducationalNotes() : value() {
    for (auto i = 0; i < N; ++i)
      value[i] = {};
#define EDUCATIONAL_NOTES(DIAG, ...)                                     \
  value[LocalDiagID::DIAG] = DIAG##_educationalNotes;
#include <w2n/AST/EducationalNotes.def>
  }

  const char * const * value[N];
};

static constexpr EducationalNotes<LocalDiagID::NumDiags>
  _EducationalNotes = EducationalNotes<LocalDiagID::NumDiags>();
static constexpr auto educationalNotes = _EducationalNotes.value;

DiagnosticState::DiagnosticState() {
  // Initialize our ignored diagnostics to default
  ignoredDiagnostics.resize(LocalDiagID::NumDiags);
}

static CharSourceRange
toCharSourceRange(SourceManager& SM, SourceRange SR) {
  // FIXME: return CharSourceRange(SM, SR.Start,
  // Lexer::getLocForEndOfToken(SM, SR.End));
  llvm_unreachable("not implemented.");
}

static CharSourceRange
toCharSourceRange(SourceManager& SM, SourceLoc Start, SourceLoc End) {
  return CharSourceRange(SM, Start, End);
}

/// Extract a character at \p Loc. If \p Loc is the end of the buffer,
/// return '\f'.
static char extractCharAfter(SourceManager& SM, SourceLoc Loc) {
  auto chars = SM.extractText({Loc, 1});
  return chars.empty() ? '\f' : chars[0];
}

/// Extract a character immediately before \p Loc. If \p Loc is the
/// start of the buffer, return '\f'.
static char extractCharBefore(SourceManager& SM, SourceLoc Loc) {
  // We have to be careful not to go off the front of the buffer.
  auto bufferID = SM.findBufferContainingLoc(Loc);
  auto bufferRange = SM.getRangeForBuffer(bufferID);
  if (bufferRange.getStart() == Loc)
    return '\f';
  auto chars = SM.extractText({Loc.getAdvancedLoc(-1), 1}, bufferID);
  assert(!chars.empty() && "Couldn't extractText with valid range");
  return chars[0];
}

InFlightDiagnostic& InFlightDiagnostic::highlight(SourceRange R) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (Engine && R.isValid())
    Engine->getActiveDiagnostic().addRange(
      toCharSourceRange(Engine->SourceMgr, R)
    );
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::highlightChars(SourceLoc Start, SourceLoc End) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (Engine && Start.isValid())
    Engine->getActiveDiagnostic().addRange(
      toCharSourceRange(Engine->SourceMgr, Start, End)
    );
  return *this;
}

/// Add an insertion fix-it to the currently-active diagnostic.  The
/// text is inserted immediately *after* the token specified.
///
InFlightDiagnostic& InFlightDiagnostic::fixItInsertAfter(
  SourceLoc L,
  StringRef FormatString,
  ArrayRef<DiagnosticArgument> Args
) {
  // L = Lexer::getLocForEndOfToken(Engine->SourceMgr, L);
  // return fixItInsert(L, FormatString, Args);
  llvm_unreachable("not implemented.");
}

/// Add a token-based removal fix-it to the currently-active
/// diagnostic.
InFlightDiagnostic& InFlightDiagnostic::fixItRemove(SourceRange R) {
  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (R.isInvalid() || !Engine)
    return *this;

  // Convert from a token range to a CharSourceRange, which points to the
  // end of the token we want to remove.
  auto& SM = Engine->SourceMgr;
  auto charRange = toCharSourceRange(SM, R);

  // If we're removing something (e.g. a keyword), do a bit of extra work
  // to make sure that we leave the code in a good place, without
  // extraneous white space around its hole.  Specifically, check to see
  // there is whitespace before and after the end of range.  If so, nuke
  // the space afterward to keep things consistent.
  if (
    extractCharAfter(SM, charRange.getEnd()) == ' ' &&
    isspace(extractCharBefore(SM, charRange.getStart()))) {
    charRange = CharSourceRange(
      charRange.getStart(), charRange.getByteLength() + 1
    );
  }
  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(charRange, {}, {})
  );
  return *this;
}

InFlightDiagnostic& InFlightDiagnostic::fixItReplace(
  SourceRange R,
  StringRef FormatString,
  ArrayRef<DiagnosticArgument> Args
) {
  auto& SM = Engine->SourceMgr;
  auto charRange = toCharSourceRange(SM, R);

  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(charRange, FormatString, Args)
  );
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::fixItReplace(SourceRange R, StringRef Str) {
  if (Str.empty())
    return fixItRemove(R);

  assert(IsActive && "Cannot modify an inactive diagnostic");
  if (R.isInvalid() || !Engine)
    return *this;

  auto& SM = Engine->SourceMgr;
  auto charRange = toCharSourceRange(SM, R);

  // If we're replacing with something that wants spaces around it, do a
  // bit of extra work so that we don't suggest extra spaces.
  // FIXME: This could probably be applied to structured fix-its as well.
  if (Str.back() == ' ') {
    if (isspace(extractCharAfter(SM, charRange.getEnd())))
      Str = Str.drop_back();
  }
  if (!Str.empty() && Str.front() == ' ') {
    if (isspace(extractCharBefore(SM, charRange.getStart())))
      Str = Str.drop_front();
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
  if (Engine && Start.isValid())
    Engine->getActiveDiagnostic().addFixIt(Diagnostic::FixIt(
      toCharSourceRange(Engine->SourceMgr, Start, End), FormatString, Args
    ));
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::fixItExchange(SourceRange R1, SourceRange R2) {
  assert(IsActive && "Cannot modify an inactive diagnostic");

  auto& SM = Engine->SourceMgr;
  // Convert from a token range to a CharSourceRange
  auto charRange1 = toCharSourceRange(SM, R1);
  auto charRange2 = toCharSourceRange(SM, R2);
  // Extract source text.
  auto text1 = SM.extractText(charRange1);
  auto text2 = SM.extractText(charRange2);

  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(charRange1, "%0", {text2})
  );
  Engine->getActiveDiagnostic().addFixIt(
    Diagnostic::FixIt(charRange2, "%0", {text1})
  );
  return *this;
}

InFlightDiagnostic&
InFlightDiagnostic::limitBehavior(DiagnosticBehavior limit) {
  Engine->getActiveDiagnostic().setBehaviorLimit(limit);
  return *this;
}

InFlightDiagnostic& InFlightDiagnostic::wrapIn(const Diagnostic& wrapper
) {
  // Save current active diagnostic into WrappedDiagnostics, ignoring
  // state so we don't get a None return or influence future diagnostics.
  DiagnosticState tempState;
  Engine->state.swap(tempState);
  llvm::SaveAndRestore<DiagnosticBehavior> limit(
    Engine->getActiveDiagnostic().BehaviorLimit,
    DiagnosticBehavior::Unspecified
  );

  Engine->WrappedDiagnostics.push_back(
    *Engine->diagnosticInfoForDiagnostic(Engine->getActiveDiagnostic())
  );

  Engine->state.swap(tempState);

  auto& wrapped = Engine->WrappedDiagnostics.back();

  // Copy and update its arg list.
  Engine->WrappedDiagnosticArgs.emplace_back(wrapped.FormatArgs);
  wrapped.FormatArgs = Engine->WrappedDiagnosticArgs.back();

  // Overwrite the ID and argument with those from the wrapper.
  Engine->getActiveDiagnostic().ID = wrapper.ID;
  Engine->getActiveDiagnostic().Args = wrapper.Args;

  // Set the argument to the diagnostic being wrapped.
  assert(
    wrapper.getArgs().front().getKind() ==
    DiagnosticArgumentKind::Diagnostic
  );
  Engine->getActiveDiagnostic().Args.front() = &wrapped;

  return *this;
}

void InFlightDiagnostic::flush() {
  if (!IsActive)
    return;

  IsActive = false;
  if (Engine)
    Engine->flushActiveDiagnostic();
}

void Diagnostic::addChildNote(Diagnostic&& D) {
  assert(
    storedDiagnosticInfos[(unsigned)D.ID].kind == DiagnosticKind::Note &&
    "Only notes can have a parent."
  );
  assert(
    storedDiagnosticInfos[(unsigned)ID].kind != DiagnosticKind::Note &&
    "Notes can't have children."
  );
  ChildNotes.push_back(std::move(D));
}

bool DiagnosticEngine::isDiagnosticPointsToFirstBadToken(DiagID ID
) const {
  return storedDiagnosticInfos[(unsigned)ID].pointsToFirstBadToken;
}

bool DiagnosticEngine::isAPIDigesterBreakageDiagnostic(DiagID ID) const {
  return storedDiagnosticInfos[(unsigned)ID].isAPIDigesterBreakage;
}

bool DiagnosticEngine::isDeprecationDiagnostic(DiagID ID) const {
  return storedDiagnosticInfos[(unsigned)ID].isDeprecation;
}

bool DiagnosticEngine::isNoUsageDiagnostic(DiagID ID) const {
  return storedDiagnosticInfos[(unsigned)ID].isNoUsage;
}

bool DiagnosticEngine::finishProcessing() {
  bool hadError = false;
  for (auto& Consumer : Consumers) {
    hadError |= Consumer->finishProcessing();
  }
  return hadError;
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
  StringRef& Text,
  char Delim,
  bool * FoundDelim = nullptr
) {
  unsigned Depth = 0;
  if (FoundDelim)
    *FoundDelim = false;

  unsigned I = 0;
  for (unsigned N = Text.size(); I != N; ++I) {
    if (Text[I] == '{') {
      ++Depth;
      continue;
    }
    if (Depth > 0) {
      if (Text[I] == '}')
        --Depth;
      continue;
    }

    if (Text[I] == Delim) {
      if (FoundDelim)
        *FoundDelim = true;
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
  llvm::raw_ostream& Out
) {
  bool foundPipe = false;
  do {
    assert(
      (!ModifierArguments.empty() || foundPipe) &&
      "Index beyond bounds in %select modifier"
    );
    StringRef Text = skipToDelimiter(ModifierArguments, '|', &foundPipe);
    if (SelectedIndex == 0) {
      DiagnosticEngine::formatDiagnosticText(Out, Text, Args, FormatOpts);
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
  llvm::raw_ostream& Out
) {
  const DiagnosticArgument& Arg = Args[ArgIndex];
  switch (Arg.getKind()) {
  case DiagnosticArgumentKind::Integer:
    if (Modifier == "select") {
      assert(Arg.getAsInteger() >= 0 && "Negative selection index");
      formatSelectionArgument(
        ModifierArguments, Args, Arg.getAsInteger(), FormatOpts, Out
      );
    } else if (Modifier == "s") {
      if (Arg.getAsInteger() != 1)
        Out << 's';
    } else {
      assert(
        Modifier.empty() && "Improper modifier for integer argument"
      );
      Out << Arg.getAsInteger();
    }
    break;

  case DiagnosticArgumentKind::Unsigned:
    if (Modifier == "select") {
      formatSelectionArgument(
        ModifierArguments, Args, Arg.getAsUnsigned(), FormatOpts, Out
      );
    } else if (Modifier == "s") {
      if (Arg.getAsUnsigned() != 1)
        Out << 's';
    } else {
      assert(
        Modifier.empty() && "Improper modifier for unsigned argument"
      );
      Out << Arg.getAsUnsigned();
    }
    break;

  case DiagnosticArgumentKind::String:
    if (Modifier == "select") {
      formatSelectionArgument(
        ModifierArguments, Args, Arg.getAsString().empty() ? 0 : 1,
        FormatOpts, Out
      );
    } else {
      assert(Modifier.empty() && "Improper modifier for string argument");
      Out << Arg.getAsString();
    }
    break;

  case DiagnosticArgumentKind::Diagnostic: {
    assert(
      Modifier.empty() && "Improper modifier for Diagnostic argument"
    );
    auto diagArg = Arg.getAsDiagnostic();
    DiagnosticEngine::formatDiagnosticText(
      Out, diagArg->FormatString, diagArg->FormatArgs
    );
    break;
  }
  }
}

/// Format the given diagnostic text and place the result in the given
/// buffer.
void DiagnosticEngine::formatDiagnosticText(
  llvm::raw_ostream& Out,
  StringRef InText,
  ArrayRef<DiagnosticArgument> Args,
  DiagnosticFormatOptions FormatOpts
) {
  while (!InText.empty()) {
    size_t Percent = InText.find('%');
    if (Percent == StringRef::npos) {
      // Write the rest of the string; we're done.
      Out.write(InText.data(), InText.size());
      break;
    }

    // Write the string up to (but not including) the %, then drop that
    // text (including the %).
    Out.write(InText.data(), Percent);
    InText = InText.substr(Percent + 1);

    // '%%' -> '%'.
    if (InText[0] == '%') {
      Out.write('%');
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
      Out << StringRef(
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
    bool IndexParseFailed =
      InText.substr(0, Length).getAsInteger(10, ArgIndex);

    if (IndexParseFailed) {
      Out << StringRef("<<INTERNAL ERROR: unparseable argument index in "
                       "diagnostic text>>");
      continue;
    }

    InText = InText.substr(Length);

    if (ArgIndex >= Args.size()) {
      Out << StringRef("<<INTERNAL ERROR: out-of-range argument index in "
                       "diagnostic text>>");
      continue;
    }

    // Convert the argument to a string.
    formatDiagnosticArgument(
      Modifier, ModifierArguments, Args, ArgIndex, FormatOpts, Out
    );
  }
}

static DiagnosticKind toDiagnosticKind(DiagnosticBehavior behavior) {
  switch (behavior) {
  case DiagnosticBehavior::Unspecified:
    llvm_unreachable("unspecified behavior");
  case DiagnosticBehavior::Ignore:
    llvm_unreachable("trying to map an ignored diagnostic");
  case DiagnosticBehavior::Error:
  case DiagnosticBehavior::Fatal:
    return DiagnosticKind::Error;
  case DiagnosticBehavior::Note:
    return DiagnosticKind::Note;
  case DiagnosticBehavior::Warning:
    return DiagnosticKind::Warning;
  case DiagnosticBehavior::Remark:
    return DiagnosticKind::Remark;
  }

  llvm_unreachable("Unhandled DiagnosticKind in switch.");
}

static DiagnosticBehavior
toDiagnosticBehavior(DiagnosticKind kind, bool isFatal) {
  switch (kind) {
  case DiagnosticKind::Note:
    return DiagnosticBehavior::Note;
  case DiagnosticKind::Error:
    return isFatal ? DiagnosticBehavior::Fatal
                   : DiagnosticBehavior::Error;
  case DiagnosticKind::Warning:
    return DiagnosticBehavior::Warning;
  case DiagnosticKind::Remark:
    return DiagnosticBehavior::Remark;
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
  "w2n-diagnostics-assert-on-warning",
  llvm::cl::init(false)
);

DiagnosticBehavior
DiagnosticState::determineBehavior(const Diagnostic& diag) {
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
  auto diagInfo = storedDiagnosticInfos[(unsigned)diag.getID()];
  DiagnosticBehavior lvl = std::max(
    toDiagnosticBehavior(diagInfo.kind, diagInfo.isFatal),
    diag.getBehaviorLimit()
  );
  assert(lvl != DiagnosticBehavior::Unspecified);

  //   2) If current state dictates a certain behavior, follow that

  // Notes relating to ignored diagnostics should also be ignored
  if (previousBehavior == DiagnosticBehavior::Ignore && lvl == DiagnosticBehavior::Note)
    lvl = DiagnosticBehavior::Ignore;

  // Suppress diagnostics when in a fatal state, except for follow-on
  // notes
  if (fatalErrorOccurred)
    if (!showDiagnosticsAfterFatalError && lvl != DiagnosticBehavior::Note)
      lvl = DiagnosticBehavior::Ignore;

  //   3) If the user ignored this specific diagnostic, follow that
  if (ignoredDiagnostics[(unsigned)diag.getID()])
    lvl = DiagnosticBehavior::Ignore;

  //   4) If the user substituted a different behavior for this behavior,
  //   apply
  //      that change
  if (lvl == DiagnosticBehavior::Warning) {
    if (warningsAsErrors)
      lvl = DiagnosticBehavior::Error;
    if (suppressWarnings)
      lvl = DiagnosticBehavior::Ignore;
  }

  //   5) Update current state for use during the next diagnostic
  if (lvl == DiagnosticBehavior::Fatal) {
    fatalErrorOccurred = true;
    anyErrorOccurred = true;
  } else if (lvl == DiagnosticBehavior::Error) {
    anyErrorOccurred = true;
  }

  assert(
    (!AssertOnError || !anyErrorOccurred) && "We emitted an error?!"
  );
  assert(
    (!AssertOnWarning || (lvl != DiagnosticBehavior::Warning)) &&
    "We emitted a warning?!"
  );

  previousBehavior = lvl;
  return lvl;
}

void DiagnosticEngine::flushActiveDiagnostic() {
  assert(ActiveDiagnostic && "No active diagnostic to flush");
  handleDiagnostic(std::move(*ActiveDiagnostic));
  ActiveDiagnostic.reset();
}

void DiagnosticEngine::handleDiagnostic(Diagnostic&& diag) {
  if (TransactionCount == 0) {
    emitDiagnostic(diag);
    WrappedDiagnostics.clear();
    WrappedDiagnosticArgs.clear();
  } else {
    onTentativeDiagnosticFlush(diag);
    TentativeDiagnostics.emplace_back(std::move(diag));
  }
}

void DiagnosticEngine::clearTentativeDiagnostics() {
  TentativeDiagnostics.clear();
  WrappedDiagnostics.clear();
  WrappedDiagnosticArgs.clear();
}

void DiagnosticEngine::emitTentativeDiagnostics() {
  for (auto& diag : TentativeDiagnostics) {
    emitDiagnostic(diag);
  }
  clearTentativeDiagnostics();
}

void DiagnosticEngine::forwardTentativeDiagnosticsTo(
  DiagnosticEngine& targetEngine
) {
  for (auto& diag : TentativeDiagnostics) {
    targetEngine.handleDiagnostic(std::move(diag));
  }
  clearTentativeDiagnostics();
}

Optional<DiagnosticInfo>
DiagnosticEngine::diagnosticInfoForDiagnostic(const Diagnostic& diagnostic
) {
  auto behavior = state.determineBehavior(diagnostic);
  if (behavior == DiagnosticBehavior::Ignore)
    return None;

  // Figure out the source location.
  SourceLoc loc = diagnostic.getLoc();
  if (loc.isInvalid() && diagnostic.getDecl()) {
    const Decl * decl = diagnostic.getDecl();
    // If a declaration was provided instead of a location, and that
    // declaration has a location we can point to, use that location.
    loc = decl->getLoc();

    if (loc.isInvalid()) {
      // There is no location we can point to. Pretty-print the
      // declaration so we can point to it.
      llvm_unreachable(
        "pretty-print for WebAseembly file is not implemented."
      );
    }
  }

  StringRef Category;
  if (isAPIDigesterBreakageDiagnostic(diagnostic.getID()))
    Category = "api-digester-breaking-change";
  else if (isDeprecationDiagnostic(diagnostic.getID()))
    Category = "deprecation";
  else if (isNoUsageDiagnostic(diagnostic.getID()))
    Category = "no-usage";

  return DiagnosticInfo(
    diagnostic.getID(), loc, toDiagnosticKind(behavior),
    diagnosticStringFor(diagnostic.getID(), getPrintDiagnosticNames()),
    diagnostic.getArgs(), Category, getDefaultDiagnosticLoc(),
    /*child note info*/ {}, diagnostic.getRanges(),
    diagnostic.getFixIts(), diagnostic.isChildNote()
  );
}

void DiagnosticEngine::emitDiagnostic(const Diagnostic& diagnostic) {
  if (auto info = diagnosticInfoForDiagnostic(diagnostic)) {
    SmallVector<DiagnosticInfo, 1> childInfo;
    auto childNotes = diagnostic.getChildNotes();
    for (unsigned i : indices(childNotes)) {
      auto child = diagnosticInfoForDiagnostic(childNotes[i]);
      assert(child);
      assert(
        child->Kind == DiagnosticKind::Note &&
        "Expected child diagnostics to all be notes?!"
      );
      childInfo.push_back(*child);
    }
    TinyPtrVector<DiagnosticInfo *> childInfoPtrs;
    for (unsigned i : indices(childInfo)) {
      childInfoPtrs.push_back(&childInfo[i]);
    }
    info->ChildDiagnosticInfo = childInfoPtrs;

    SmallVector<std::string, 1> educationalNotePaths;
    auto associatedNotes = educationalNotes[(uint32_t)diagnostic.getID()];
    while (associatedNotes && *associatedNotes) {
      SmallString<128> notePath(getDiagnosticDocumentationPath());
      llvm::sys::path::append(notePath, *associatedNotes);
      educationalNotePaths.push_back(notePath.str().str());
      ++associatedNotes;
    }
    info->EducationalNotePaths = educationalNotePaths;

    for (auto& consumer : Consumers) {
      consumer->handleDiagnostic(SourceMgr, *info);
    }
  }

  // For compatibility with DiagnosticConsumers which don't know about
  // child notes. These can be ignored by consumers which do take
  // advantage of the grouping.
  for (auto& childNote : diagnostic.getChildNotes())
    emitDiagnostic(childNote);
}

DiagnosticKind DiagnosticEngine::declaredDiagnosticKindFor(const DiagID id
) {
  return storedDiagnosticInfos[(unsigned)id].kind;
}

llvm::StringRef DiagnosticEngine::diagnosticStringFor(
  const DiagID id,
  bool printDiagnosticNames
) {
  auto defaultMessage = printDiagnosticNames
                          ? debugDiagnosticStrings[(unsigned)id]
                          : diagnosticStrings[(unsigned)id];

  if (auto producer = localization.get()) {
    auto localizedMessage = producer->getMessageOr(id, defaultMessage);
    return localizedMessage;
  }
  return defaultMessage;
}

llvm::StringRef DiagnosticEngine::diagnosticIDStringFor(const DiagID id) {
  return diagnosticIDStrings[(unsigned)id];
}

const char * InFlightDiagnostic::fixItStringFor(const FixItID id) {
  return fixItStrings[(unsigned)id];
}

void DiagnosticEngine::setBufferIndirectlyCausingDiagnosticToInput(
  SourceLoc loc
) {
  // If in the future, nested BufferIndirectlyCausingDiagnosticRAII need
  // be supported, the compiler will need a stack for
  // bufferIndirectlyCausingDiagnostic.
  assert(
    bufferIndirectlyCausingDiagnostic.isInvalid() &&
    "Buffer should not already be set."
  );
  bufferIndirectlyCausingDiagnostic = loc;
  assert(
    bufferIndirectlyCausingDiagnostic.isValid() &&
    "Buffer must be valid for previous assertion to work."
  );
}

void DiagnosticEngine::resetBufferIndirectlyCausingDiagnostic() {
  bufferIndirectlyCausingDiagnostic = SourceLoc();
}

DiagnosticSuppression::DiagnosticSuppression(DiagnosticEngine& diags)
  : diags(diags) {
  consumers = diags.takeConsumers();
}

DiagnosticSuppression::~DiagnosticSuppression() {
  for (auto consumer : consumers)
    diags.addConsumer(*consumer);
}

bool DiagnosticSuppression::isEnabled(const DiagnosticEngine& diags) {
  return diags.getConsumers().empty();
}

BufferIndirectlyCausingDiagnosticRAII::
  BufferIndirectlyCausingDiagnosticRAII(const SourceFile& SF)
  : Diags(SF.getASTContext().Diags) {
  auto id = SF.getBufferID();
  if (!id)
    return;
  auto loc = SF.getASTContext().SourceMgr.getLocForBufferStart(*id);
  if (loc.isValid())
    Diags.setBufferIndirectlyCausingDiagnosticToInput(loc);
}

void DiagnosticEngine::onTentativeDiagnosticFlush(Diagnostic& diagnostic
) {
  for (auto& argument : diagnostic.Args) {
    if (argument.getKind() != DiagnosticArgumentKind::String)
      continue;

    auto content = argument.getAsString();
    if (content.empty())
      continue;

    auto I = TransactionStrings.insert(content).first;
    argument = DiagnosticArgument(StringRef(I->getKeyData()));
  }
}
