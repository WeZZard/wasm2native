#ifndef W2N_AST_FILEUNIT_H
#define W2N_AST_FILEUNIT_H

namespace w2n {

/// Discriminator for file-units.
enum class FileUnitKind {
  /// For a .wasm binary file.
  Wasm
};

class FileUnit {};

class WasmFile : public FileUnit {};

} // namespace w2n

#endif // W2N_AST_FILEUNIT_H