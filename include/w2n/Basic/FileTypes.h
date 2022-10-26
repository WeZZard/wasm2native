#ifndef W2N_BASIC_FILETYPES_H
#define W2N_BASIC_FILETYPES_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {
namespace file_types {
enum ID : uint8_t {

#define TYPE(NAME, ID, EXTENSION, FLAGS) TY_##ID,
#include <w2n/Basic/FileTypes.def>
#undef TYPE
  TY_INVALID
};

/// Return the name of the type for \p Id.
StringRef getTypeName(ID Id);

/// Return the extension to use when creating a file of this type,
/// or an empty string if unspecified.
StringRef getExtension(ID Id);

/// Lookup the type to use for the file extension \p Ext.
/// If the extension is empty or is otherwise not recognized, return
/// the invalid type \c TY_INVALID.
ID lookupTypeForExtension(StringRef Ext);

/// Lookup the type to use for the name \p Name.
ID lookupTypeForName(StringRef Name);

static inline void forAllTypes(llvm::function_ref<void(file_types::ID)> fn
) {
  for (uint8_t i = 0; i < static_cast<uint8_t>(TY_INVALID); ++i)
    fn(static_cast<ID>(i));
}

bool isInputType(ID Id);

} // end namespace file_types
} // end namespace w2n

namespace llvm {
template <>
struct DenseMapInfo<w2n::file_types::ID> {
  using ID = w2n::file_types::ID;

  static inline ID getEmptyKey() {
    return ID::TY_INVALID;
  }

  static inline ID getTombstoneKey() {
    return static_cast<ID>(ID::TY_INVALID + 1);
  }

  static unsigned getHashValue(ID Val) {
    return (unsigned)Val * 37U;
  }

  static bool isEqual(ID LHS, ID RHS) {
    return LHS == RHS;
  }
};
} // end namespace llvm

#endif // W2N_BASIC_FILETYPES_H
