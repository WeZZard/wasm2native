#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Wasm.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/SymbolicFile.h>
#include <llvm/Object/Wasm.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/LEB128.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <cstddef>
#include <memory>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/ImplicitTrailingObject.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/Format/Format.h>
#include <w2n/Parse/Parser.h>

using namespace w2n;
using namespace llvm::object;

/// Current implementation of \c WasmParser is ripped from LLVM's
/// implementation of WebAssembly object file support. There are a lot of
/// room to improve the performance and the organization.

#define W2N_VARIN_T7_MAX  ((1 << 7) - 1)
#define W2N_VARIN_T7_MIN  (-(1 << 7))
#define W2N_VARUIN_T7_MAX (1 << 7)
#define W2N_VARUIN_T1_MAX (1)

struct ReadContext {
  const uint8_t * Start;
  const uint8_t * Ptr;
  const uint8_t * End;
};

static uint8_t readUint8(ReadContext& Ctx) {
  if (Ctx.Ptr == Ctx.End) {
    llvm_unreachable("EOF while reading uint8");
  }
  return *Ctx.Ptr++;
}

static uint32_t readUint32(ReadContext& Ctx) {
  if (Ctx.Ptr + 4 > Ctx.End) {
    llvm_unreachable("EOF while reading uint32");
  }
  uint32_t Result = llvm::support::endian::read32le(Ctx.Ptr);
  Ctx.Ptr += 4;
  return Result;
}

static int32_t readFloat32(ReadContext& Ctx) {
  if (Ctx.Ptr + 4 > Ctx.End) {
    llvm_unreachable("EOF while reading float64");
  }
  int32_t Result = 0;
  memcpy(&Result, Ctx.Ptr, sizeof(Result));
  Ctx.Ptr += sizeof(Result);
  return Result;
}

static int64_t readFloat64(ReadContext& Ctx) {
  if (Ctx.Ptr + 8 > Ctx.End) {
    llvm_unreachable("EOF while reading float64");
  }
  int64_t Result = 0;
  memcpy(&Result, Ctx.Ptr, sizeof(Result));
  Ctx.Ptr += sizeof(Result);
  return Result;
}

static uint64_t readULEB128(ReadContext& Ctx) {
  unsigned Count;
  const char * Error = nullptr;
  uint64_t Result = llvm::decodeULEB128(Ctx.Ptr, &Count, Ctx.End, &Error);
  if (Error != nullptr) {
    llvm_unreachable(Error);
  }
  Ctx.Ptr += Count;
  return Result;
}

static StringRef readString(ReadContext& Ctx) {
  uint32_t StringLen = readULEB128(Ctx);
  if (Ctx.Ptr + StringLen > Ctx.End) {
    llvm_unreachable("EOF while reading string");
  }
  StringRef Return =
    StringRef(reinterpret_cast<const char *>(Ctx.Ptr), StringLen);
  Ctx.Ptr += StringLen;
  return Return;
}

static int64_t readLEB128(ReadContext& Ctx) {
  unsigned Count;
  const char * Error = nullptr;
  uint64_t Result = llvm::decodeSLEB128(Ctx.Ptr, &Count, Ctx.End, &Error);
  if (Error != nullptr) {
    llvm_unreachable(Error);
  }
  Ctx.Ptr += Count;
  return Result;
}

static uint8_t readVaruint1(ReadContext& Ctx) {
  int64_t Result = readLEB128(Ctx);
  if (Result > W2N_VARUIN_T1_MAX || Result < 0) {
    llvm_unreachable("LEB is outside Varuint1 range");
  }
  return Result;
}

static int32_t readVarint32(ReadContext& Ctx) {
  int64_t Result = readLEB128(Ctx);
  if (Result > INT32_MAX || Result < INT32_MIN) {
    llvm_unreachable("LEB is outside Varint32 range");
  }
  return Result;
}

static uint32_t readVaruint32(ReadContext& Ctx) {
  uint64_t Result = readULEB128(Ctx);
  if (Result > UINT32_MAX) {
    llvm_unreachable("LEB is outside Varuint32 range");
  }
  return Result;
}

static int64_t readVarint64(ReadContext& Ctx) {
  return readLEB128(Ctx);
}

static uint64_t readVaruint64(ReadContext& Ctx) {
  return readULEB128(Ctx);
}

static uint8_t readOpcode(ReadContext& Ctx) {
  return readUint8(Ctx);
}

static TypeKind getTypeKind(unsigned RawType) {
  switch (RawType) {
  case llvm::wasm::WASM_TYPE_I32: return TypeKind::I32;
  case llvm::wasm::WASM_TYPE_I64: return TypeKind::I64;
  case llvm::wasm::WASM_TYPE_F32: return TypeKind::F32;
  case llvm::wasm::WASM_TYPE_F64: return TypeKind::F64;
  case llvm::wasm::WASM_TYPE_V128: return TypeKind::V128;
  case llvm::wasm::WASM_TYPE_FUNCREF: return TypeKind::FuncRef;
  case llvm::wasm::WASM_TYPE_EXTERNREF: return TypeKind::ExternRef;
  default: llvm_unreachable("Invalid raw type.");
  }
}

class WasmParser::Implementation {
private:

public:

  WasmParser * Parser;

  std::unique_ptr<WasmObjectFile> WasmObject;

  explicit Implementation(WasmParser * Parser) : Parser(Parser) {
  }

  ASTContext& getContext() {
    return Parser->File.getASTContext();
  }

  const ASTContext& getContext() const {
    return Parser->File.getASTContext();
  }

  /**
   * @brief \c ModuleDecl in .wasm format is an imaginary AST node since
   * there is no actual content can be parsed as \c ModuleDecl .
   *
   * @note
   * \verbatim
   *  module-decl:
   *    section-decl*
   * \endverbatim
   */
  ModuleDecl * parseModuleDecl() {
    llvm::SmallVector<SectionDecl *> SectionDecls;
    parseSectionDecls(SectionDecls);
    const char * Filename = Parser->File.getFilename().data();
    Identifier ModuleName = Identifier::getFromOpaquePointer(
      reinterpret_cast<void *>(const_cast<char *>(Filename))
    );
    ModuleDecl * Mod =
      ModuleDecl::create(ModuleName, Parser->File.getASTContext());
    for (auto& EachSectionDecl : SectionDecls) {
      Mod->addSectionDecl(EachSectionDecl);
    }
    return Mod;
  }

  CustomSectionDecl *
  parseCustomSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  TypeSectionDecl *
  parseTypeSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<FuncTypeDecl *> FuncTypeDecls;
    FuncTypeDecls.reserve(Count);
    while (Count-- != 0) {
      uint8_t Form = readUint8(Ctx);
      if (Form != llvm::wasm::WASM_TYPE_FUNC) {
        llvm_unreachable("invalid signature type");
      }
      uint32_t ParamCount = readVaruint32(Ctx);
      llvm::SmallVector<Type *, 4> Params;
      Params.reserve(ParamCount);
      while (ParamCount-- != 0) {
        uint32_t ParamType = readUint8(Ctx);
        TypeKind Kind = getTypeKind(ParamType);
        Type * Type = getContext().getTypeForKind(Kind);
        Params.push_back(Type);
      }
      uint32_t ReturnCount = readVaruint32(Ctx);
      llvm::SmallVector<Type *, 1> Returns;
      while (ReturnCount-- != 0) {
        uint32_t ReturnType = readUint8(Ctx);
        TypeKind Kind = getTypeKind(ReturnType);
        Type * Type = getContext().getTypeForKind(Kind);
        Returns.push_back(Type);
      }
      FuncTypeDecl * FuncTypeDecl =
        FuncTypeDecl::create(getContext(), Params, Returns);
      FuncTypeDecls.push_back(std::move(FuncTypeDecl));
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("type section ended prematurely");
    }
    return TypeSectionDecl::create(getContext(), FuncTypeDecls);
  }

  ImportSectionDecl *
  parseImportSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  FuncSectionDecl *
  parseFuncSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  TableSectionDecl *
  parseTableSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  MemorySectionDecl *
  parseMemorySectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  GlobalSectionDecl *
  parseGlobalSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  ExportSectionDecl *
  parseExportSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  StartSectionDecl *
  parseStartSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  ElementSectionDecl *
  parseElementSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CodeSectionDecl *
  parseCodeSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  DataSectionDecl *
  parseDataSectionDecl(const WasmSection& Section, ReadContext& Ctx) {
    w2n_unimplemented();
  }

  DataCountSectionDecl * parseDataCountSectionDecl(
    const WasmSection& Section, ReadContext& Ctx
  ) {
    w2n_unimplemented();
  }

  /**
   * @brief
   *
   * @note
   * \verbatim
   *  section-decl:
   *
   * \endverbatim
   */
  SectionDecl * parseSectionDecl(const WasmSection& Section) {
    ReadContext Ctx;
    Ctx.Start = Section.Content.data();
    Ctx.End = Ctx.Start + Section.Content.size();
    Ctx.Ptr = Ctx.Start;

#define DECL(Id, Parent)
#define SECTION_DECL(Id, _)                                              \
  case W2N_FORMAT_GET_SEC_TYPE(Id): return parse##Id##Decl(Section, Ctx);
    switch (Section.Type) {
#include <w2n/AST/DeclNodes.def>
    case llvm::wasm::WASM_SEC_TAG:
      llvm_unreachable("Tag section is not supported yet.");
      break;
    }
    llvm_unreachable("unknown section type");
  }

  void parseSectionDecls(llvm::SmallVectorImpl<SectionDecl *>& Sections) {
    section_iterator CurrentSect = WasmObject->section_begin();
    const section_iterator SectEnd = WasmObject->section_end();
    while (CurrentSect != SectEnd) {
      const auto& Sec = WasmObject->getWasmSection(*CurrentSect);
      SectionDecl * SectionDecl = parseSectionDecl(Sec);
      if (SectionDecl != nullptr) {
        Sections.push_back(SectionDecl);
      }
      CurrentSect = CurrentSect.operator++();
    }
  }
};

WasmParser::Implementation& WasmParser::getImpl() const {
  return getImplicitTrailingObject<WasmParser, Implementation>(this);
}

WasmParser::WasmParser(
  unsigned BufferID, SourceFile& SF, DiagnosticEngine * LexerDiags
) :
  BufferID(BufferID),
  File(SF),
  SourceMgr(SF.getASTContext().SourceMgr),
  LexerDiags(LexerDiags) {
  StringRef Identifier = SF.getFilename();
  StringRef Contents =
    SourceMgr.extractText(SourceMgr.getRangeForBuffer(BufferID));
  auto Object = llvm::MemoryBufferRef(Contents, Identifier);
  auto ObjectFile =
    llvm::cantFail(ObjectFile::createWasmObjectFile(Object));
  getImpl().WasmObject = std::move(ObjectFile);
}

std::unique_ptr<WasmParser> WasmParser::createWasmParser(
  unsigned BufferID, SourceFile& SF, DiagnosticEngine * LexerDiags
) {
  WasmParser * Body;
  Implementation * Impl;
  std::tie(Body, Impl) =
    alignedAllocWithImplicitTrailingObject<WasmParser, Implementation>();
  new (Impl) Implementation(Body);
  return std::unique_ptr<WasmParser>(new (Body
  ) WasmParser(BufferID, SF, LexerDiags));
}

WasmParser::~WasmParser() {
  getImpl().~Implementation();
}

/**
 * @brief Main entrypoint for the parser.
 *
 * @param Decls
 *
 * @note
 * \verbatim
 *  top-level:
 *    module-decl
 * \endverbatim
 */
void WasmParser::parseTopLevel(llvm::SmallVectorImpl<Decl *>& Decls) {
  auto * Mod = getImpl().parseModuleDecl();
  if (Mod != nullptr) {
    Decls.push_back(Mod);
  }
}
