#include <w2n/Basic/FileTypes.h>

// #include <w2n/Strings.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/ErrorHandling.h>

using namespace w2n;
using namespace w2n::file_types;

namespace {
struct TypeInfo {
  const char * Name;
  const char * Flags;
  const char * Extension;
};
} // end anonymous namespace

static const TypeInfo TypeInfos[] = {
#define TYPE(NAME, ID, EXTENSION, FLAGS) {NAME, FLAGS, EXTENSION},
#include <w2n/Basic/FileTypes.def>
};

static const TypeInfo& getInfo(unsigned Id) {
  assert(Id >= 0 && Id < TY_INVALID && "Invalid Type ID.");
  return TypeInfos[Id];
}

StringRef file_types::getTypeName(ID Id) {
  return getInfo(Id).Name;
}

StringRef file_types::getExtension(ID Id) {
  return getInfo(Id).Extension;
}

ID file_types::lookupTypeForExtension(StringRef Ext) {
  if (Ext.empty())
    return TY_INVALID;
  assert(Ext.front() == '.' && "not a file extension");
  return llvm::StringSwitch<file_types::ID>(Ext.drop_front())
#define TYPE(NAME, ID, EXTENSION, FLAGS) .Case(EXTENSION, TY_##ID)
#include <w2n/Basic/FileTypes.def>
    .Default(TY_INVALID);
}

ID file_types::lookupTypeForName(StringRef Name) {
  return llvm::StringSwitch<file_types::ID>(Name)
#define TYPE(NAME, ID, EXTENSION, FLAGS) .Case(NAME, TY_##ID)
#include <w2n/Basic/FileTypes.def>
    .Default(TY_INVALID);
}

bool file_types::isInputType(ID Id) {
  switch (Id) {
  case file_types::TY_Wasm: return true;
  case file_types::TY_Image:
  case file_types::TY_Object:
  case file_types::TY_Assembly:
  case file_types::TY_LLVM_IR:
  case file_types::TY_LLVM_BC:
  case file_types::TY_TBD:
  case file_types::TY_INVALID: return false;
  }
}
