#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/SourceLoc.h>

using namespace w2n;

// Helper functions to verify statically whether source-location
// functions have been overridden.
typedef const char (&TwoChars)[2];
template <typename Class>
inline char checkSourceLocType(SourceLoc (Class::*)() const);
inline TwoChars checkSourceLocType(SourceLoc (Decl::*)() const);

template <typename Class>
inline char checkSourceLocType(SourceLoc (Class::*)(bool) const);
inline TwoChars checkSourceLocType(SourceLoc (Decl::*)(bool) const);

template <typename Class>
inline char checkSourceRangeType(SourceRange (Class::*)() const);
inline TwoChars checkSourceRangeType(SourceRange (Decl::*)() const);

DescriptiveDeclKind Decl::getDescriptiveKind() const {
#define TRIVIAL_KIND(Kind)                                               \
  case DeclKind::Kind: return DescriptiveDeclKind::Kind

  switch (getKind()) { TRIVIAL_KIND(Module); }

#undef TRIVIAL_KIND
  llvm_unreachable("bad DescriptiveDeclKind");
}

StringRef Decl::getDescriptiveKindName(DescriptiveDeclKind K) const {
#define ENTRY(Kind, String)                                              \
  case DescriptiveDeclKind::Kind: return String
  switch (K) { ENTRY(Module, "module"); }
#undef ENTRY
  llvm_unreachable("bad DescriptiveDeclKind");
}

void Decl::setDeclContext(DeclContext * DC) {
  Context = DC;
}

SourceLoc Decl::getLoc(bool SerializedOK) const {
#define DECL(ID, X)                                                      \
  static_assert(                                                         \
    sizeof(checkSourceLocType(&ID##Decl::getLoc)) == 2,                  \
    #ID "Decl is re-defining getLoc()"                                   \
  );
#include <w2n/AST/DeclNodes.def>
  if (isa<ModuleDecl>(this))
    return SourceLoc();
  // When the decl is context-free, we should get loc from source buffer.
  if (!getDeclContext())
    return getLocFromSource();
  FileUnit * File =
    dyn_cast<FileUnit>(getDeclContext()->getModuleScopeContext());
  if (!File)
    return getLocFromSource();
  switch (File->getKind()) {
  case FileUnitKind::Source: return getLocFromSource();
  case FileUnitKind::Builtin: return SourceLoc();
  }
  llvm_unreachable("invalid file kind");
}

SourceLoc Decl::getLocFromSource() const {
  switch (getKind()) {
#define DECL(ID, X)                                                      \
  static_assert(                                                         \
    sizeof(checkSourceLocType(&ID##Decl::getLocFromSource)) == 1,        \
    #ID "Decl is missing getLocFromSource()"                             \
  );                                                                     \
  case DeclKind::ID:                                                     \
    return cast<ID##Decl>(this)->getLocFromSource();
#include <w2n/AST/DeclNodes.def>
  }

  llvm_unreachable("Unknown decl kind");
}

void w2n::simple_display(llvm::raw_ostream& out, const Decl * decl) {
  if (!decl) {
    out << "(null)";
    return;
  }

  out << "(unknown decl)";
}

SourceLoc w2n::extractNearestSourceLoc(const Decl * D) {
  auto loc = D->getLoc(/*SerializedOK=*/false);
  if (loc.isValid())
    return loc;

  return extractNearestSourceLoc(D->getDeclContext());
}
