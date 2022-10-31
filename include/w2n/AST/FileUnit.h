#ifndef W2N_AST_FILEUNIT_H
#define W2N_AST_FILEUNIT_H

#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Module.h>

namespace w2n {

/**
 * @brief Discriminator for file-units.
 *
 */
enum class FileUnitKind {
  /**
   * @brief For a .wam or .wat file.
   */
  Source,

  /**
   * @brief For the compiler Builtin module.
   */
  Builtin,
};

/**
 * @brief A container for module-scope declarations that itself provides a
 * scope; the smallest unit of code organization.
 *
 * @note \c FileUnit is an abstract base class; its subclasses represent
 * different sorts of containers that can each provide a set of decls,
 * e.g. a source file. A module can contain several file-units.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

class FileUnit : public DeclContext, public ASTAllocated<FileUnit> {
#pragma clang diagnostic pop

private:
  FileUnitKind Kind;

protected:
  FileUnit(FileUnitKind Kind, ModuleDecl& Module)
    : DeclContext(DeclContextKind::FileUnit, &Module),
      Kind(Kind) {
  }

public:
  FileUnitKind getKind() const {
    return Kind;
  }

  static bool classof(const DeclContext * DC) {
    return DC->getContextKind() == DeclContextKind::FileUnit;
  }

  const ModuleDecl * getModule() const {
    return getParentModule();
  }

  ModuleDecl * getModule() {
    return getParentModule();
  }

  /// Generates the list of libraries needed to link this file, based on
  /// its imports.
  virtual void
  collectLinkLibraries(ModuleDecl::LinkLibraryCallback callback) const {
  }

  using ASTAllocated<FileUnit>::operator new;
  using ASTAllocated<FileUnit>::operator delete;
};

void simple_display(llvm::raw_ostream& out, const FileUnit * file);

} // namespace w2n

#endif // W2N_AST_FILEUNIT_H