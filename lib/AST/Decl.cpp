#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/Unimplemented.h>

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
#define W2N_TRIVIAL_KIND(Kind)                                           \
  case DeclKind::Kind:                                                   \
    return DescriptiveDeclKind::Kind

#define DECL(Id, _) W2N_TRIVIAL_KIND(Id);
  switch (getKind()) {
#include <w2n/AST/DeclNodes.def>
  }

#undef TRIVIAL_KIND
  llvm_unreachable("bad DescriptiveDeclKind");
}

StringRef Decl::getDescriptiveKindName(DescriptiveDeclKind K) const {
#define W2N_ENTRY(Kind, String)                                          \
  case DescriptiveDeclKind::Kind: return String
  switch (K) {
    W2N_ENTRY(Module, "module");
    W2N_ENTRY(NameSection, "custom section - name");
    W2N_ENTRY(TypeSection, "type section");
    W2N_ENTRY(ImportSection, "import section");
    W2N_ENTRY(FuncSection, "func section");
    W2N_ENTRY(TableSection, "table section");
    W2N_ENTRY(MemorySection, "memory section");
    W2N_ENTRY(GlobalSection, "global section");
    W2N_ENTRY(ExportSection, "export section");
    W2N_ENTRY(StartSection, "start section");
    W2N_ENTRY(ElementSection, "element section");
    W2N_ENTRY(CodeSection, "code section");
    W2N_ENTRY(DataSection, "data section");
    W2N_ENTRY(DataCountSection, "data count section");
    W2N_ENTRY(ModuleNameSubsection, "module name sub-section");
    W2N_ENTRY(FuncNameSubsection, "function name sub-section");
    W2N_ENTRY(LocalNameSubsection, "local name sub-section");
    W2N_ENTRY(FuncType, "function type");
    W2N_ENTRY(ImportFunc, "import function");
    W2N_ENTRY(ImportTable, "import table");
    W2N_ENTRY(ImportMemory, "import memory");
    W2N_ENTRY(ImportGlobal, "import global");
    W2N_ENTRY(Table, "table");
    W2N_ENTRY(Memory, "memory");
    W2N_ENTRY(Global, "global");
    W2N_ENTRY(ExportFunc, "export function");
    W2N_ENTRY(ExportTable, "export table");
    W2N_ENTRY(ExportMemory, "export memory");
    W2N_ENTRY(ExportGlobal, "export global");
    W2N_ENTRY(Code, "code");
    W2N_ENTRY(Func, "function");
    W2N_ENTRY(Local, "local");
    W2N_ENTRY(DataSegmentActive, "data segment - active");
    W2N_ENTRY(DataSegmentPassive, "data segment - passive");
    W2N_ENTRY(Expression, "expression");
  }
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
  if (isa<ModuleDecl>(this)) {
    return SourceLoc();
  }
  // When the decl is context-free, we should get loc from source buffer.
  if (getDeclContext() == nullptr) {
    return getLocFromSource();
  }
  FileUnit * File =
    dyn_cast<FileUnit>(getDeclContext()->getModuleScopeContext());
  if (File == nullptr) {
    return getLocFromSource();
  }
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

void Decl::dump(raw_ostream& os, unsigned Indent) const {
  w2n_proto_implemented([&] {
    simple_display(os, this);
    os << "\n";
  });
}

void w2n::simple_display(llvm::raw_ostream& os, const Decl * ss) {
  if (ss == nullptr) {
    os << "(null)";
    return;
  }
  if (const auto * ValueD = dyn_cast<ValueDecl>(ss)) {
    simple_display(os, ValueD);
  } else {
    os << "(unknown decl)";
  }
}

SourceLoc w2n::extractNearestSourceLoc(const Decl * D) {
  auto Loc = D->getLoc(/*SerializedOK=*/false);
  if (Loc.isValid()) {
    return Loc;
  }

  return extractNearestSourceLoc(D->getDeclContext());
}

#pragma mark - ValueDecl

void ValueDecl::dumpRef(raw_ostream& os) const {
  if (!isa<ModuleDecl>(this)) {
    // TODO: Print the context.
    // TODO: Print name.
  } else {
    auto ModuleName = cast<ModuleDecl>(this)->getName();
    os << ModuleName;
  }

  // TODO: Print location.
}

void w2n::simple_display(llvm::raw_ostream& os, const ValueDecl * ss) {
  if (ss != nullptr) {
    ss->dumpRef(os);
  } else {
    os << "(null)";
  }
}

#pragma mark - SectionDecl

#pragma mark - Subclasses of SectionDecl
