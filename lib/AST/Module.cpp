#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Module.h>

using namespace w2n;

ModuleDecl::ModuleDecl(Identifier Name, ASTContext& Context)
  : DeclContext(DeclContextKind::Module, nullptr),
    Decl(DeclKind::Module, const_cast<ASTContext *>(&Context)) {
  Context.addDestructorCleanup(*this);
}

Identifier ModuleDecl::getName() const { return Name; }

void ModuleDecl::addFile(FileUnit& NewFile) {
  Files.push_back(&NewFile);
  // FIXME: clearLookupCache();
}

ArrayRef<SourceFile *> ModuleDecl::getPrimarySourceFiles() const {
  return ArrayRef<SourceFile *>();
}
