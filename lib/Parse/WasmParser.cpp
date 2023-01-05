#include <_types/_uint32_t.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
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
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Expr.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/Instructions.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/Stmt.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/ImplicitTrailingObject.h>
#include <w2n/Basic/LLVM.h>
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

static ValueTypeKind getValueTypeKind(unsigned RawType) {
  switch (RawType) {
  case llvm::wasm::WASM_TYPE_I32: return ValueTypeKind::I32;
  case llvm::wasm::WASM_TYPE_I64: return ValueTypeKind::I64;
  case llvm::wasm::WASM_TYPE_F32: return ValueTypeKind::F32;
  case llvm::wasm::WASM_TYPE_F64: return ValueTypeKind::F64;
  case llvm::wasm::WASM_TYPE_V128: return ValueTypeKind::V128;
  case llvm::wasm::WASM_TYPE_FUNCREF: return ValueTypeKind::FuncRef;
  case llvm::wasm::WASM_TYPE_EXTERNREF: return ValueTypeKind::ExternRef;
  default: llvm_unreachable("Invalid raw value type.");
  }
}

class WasmParser::Implementation {
private:

public:

  WasmParser * Parser;

  std::unique_ptr<WasmObjectFile> WasmObject;

#define DECL(Id, Parent)
#define SECTION_DECL(Id, _) Id##Decl * Parsed##Id##Decl;
#include <w2n/AST/DeclNodes.def>

  uint32_t NumTypes = 0;
  uint32_t NumImportedFunctions = 0;
  uint32_t NumImportedGlobals = 0;
  uint32_t NumImportedTables = 0;
  uint32_t CodeSection = 0;
  uint32_t DataSection = 0;
  uint32_t GlobalSection = 0;
  uint32_t TableSection = 0;

  explicit Implementation(WasmParser * Parser) : Parser(Parser) {
  }

  ASTContext& getContext() {
    return Parser->File.getASTContext();
  }

  const ASTContext& getContext() const {
    return Parser->File.getASTContext();
  }

#pragma mark Parsing Types

  ValueType * parseValueType(ReadContext& Ctx) {
    uint32_t RawKind = readUint8(Ctx);
    ValueTypeKind Kind = getValueTypeKind(RawKind);
    return getContext().getValueTypeForKind(Kind);
  }

  LimitsType * parseLimits(ReadContext& Ctx) {
    uint32_t Flags = readVaruint32(Ctx);
    uint64_t Minimum = readVaruint64(Ctx);
    llvm::Optional<uint64_t> Maximum;
    if ((Flags & llvm::wasm::WASM_LIMITS_FLAG_HAS_MAX) != 0) {
      Maximum = readVaruint64(Ctx);
    }
    if ((Flags & llvm::wasm::WASM_LIMITS_FLAG_IS_64) != 0) {
      llvm_unreachable("64-bit memory is currently not supported.");
    }
    return getContext().getLimits(Minimum, Maximum);
  }

  TableType * parseTableType(ReadContext& Ctx) {
    ValueType * ElemType = parseValueType(Ctx);
    ReferenceType * ElementType = dyn_cast<ReferenceType>(ElemType);
    if (ElementType == nullptr) {
      llvm_unreachable("invalid table element type");
    }
    LimitsType * Limits = parseLimits(Ctx);
    return getContext().getTableType(ElementType, Limits);
  }

  MemoryType * parseMemoryType(ReadContext& Ctx) {
    LimitsType * Limits = parseLimits(Ctx);
    return getContext().getMemoryType(Limits);
  }

  GlobalType * parseGlobalType(ReadContext& Ctx) {
    ValueType * Type = parseValueType(Ctx);
    bool IsMutable = readVaruint1(Ctx) != 0;
    return getContext().getGlobalType(Type, IsMutable);
  }

  ResultType * parseResultType(ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<ValueType *> ValueTypes;
    ValueTypes.reserve(Count);
    while (Count-- != 0) {
      ValueType * Type = parseValueType(Ctx);
      ValueTypes.push_back(Type);
    }
    return getContext().getResultType(ValueTypes);
  }

  FuncType * parseFuncType(ReadContext& Ctx) {
    ResultType * Params = parseResultType(Ctx);
    ResultType * Returns = parseResultType(Ctx);
    return getContext().getFuncType(Params, Returns);
  }

#pragma mark Parsing Direct Section Contents

  TableDecl * parseTableDecl(ReadContext& Ctx) {
    TableType * Ty = parseTableType(Ctx);
    return TableDecl::create(getContext(), Ty);
  }

  MemoryDecl * parseMemoryDecl(ReadContext& Ctx) {
    MemoryType * Ty = parseMemoryType(Ctx);
    return MemoryDecl::create(getContext(), Ty);
  }

  GlobalDecl * parseGlobalDecl(ReadContext& Ctx) {
    GlobalType * Ty = parseGlobalType(Ctx);
    ExpressionDecl * Init = parseExpressionDecl(Ctx);
    return GlobalDecl::create(getContext(), Ty, Init);
  }

  ExportDecl * parseExportDecl(ReadContext& Ctx) {
    Identifier Name = getContext().getIdentifier(readString(Ctx));
    uint8_t Kind = readUint8(Ctx);
    uint32_t Index = readVaruint32(Ctx);
    switch (Kind) {
    case llvm::wasm::WASM_EXTERNAL_FUNCTION:
      // FIXME: !isDefinedFunctionIndex(Index) -> invalid function export
      return ExportFuncDecl::create(getContext(), Name, Index);
    case llvm::wasm::WASM_EXTERNAL_GLOBAL:
      // FIXME: !isValidGlobalIndex(Index) -> invalid global export
      return ExportGlobalDecl::create(getContext(), Name, Index);
    case llvm::wasm::WASM_EXTERNAL_TAG:
      llvm_unreachable("tag is not supported");
    case llvm::wasm::WASM_EXTERNAL_MEMORY:
      return ExportMemoryDecl::create(getContext(), Name, Index);
    case llvm::wasm::WASM_EXTERNAL_TABLE:
      return ExportTableDecl::create(getContext(), Name, Index);
    default: llvm_unreachable("unexpected export kind");
    }
  }

  CodeDecl * parseCodeDecl(ReadContext& Ctx) {
    uint32_t Size = readVaruint32(Ctx);
    FuncDecl * Func = parseFuncDecl(Ctx);
    return CodeDecl::create(getContext(), Size, Func);
  }

  FuncDecl * parseFuncDecl(ReadContext& Ctx) {
    uint32_t NumLocalDecls = readVaruint32(Ctx);
    std::vector<LocalDecl *> Locals;
    Locals.reserve(NumLocalDecls);
    while ((NumLocalDecls--) != 0) {
      LocalDecl * Local = parseLocalDecl(Ctx);
      Locals.push_back(Local);
    }
    ExpressionDecl * Expression = parseExpressionDecl(Ctx);
    // FIXME: validation
    return FuncDecl::create(getContext(), Locals, Expression);
  }

  LocalDecl * parseLocalDecl(ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    ValueType * Type = parseValueType(Ctx);
    return LocalDecl::create(getContext(), Count, Type);
  }

  ExpressionDecl * parseExpressionDecl(ReadContext& Ctx) {
    std::vector<InstNode> Instructions;
    InstNode Instruction = nullptr;
    std::cout
      << "[WasmParser::Implementation] [parseExpressionDecl] BEGAN\n";
    while (Instruction.isNull() || !Instruction.isEndStmt()) {
      Instruction = parseInstruction(Ctx);
      Instructions.push_back(Instruction);
    }
    std::cout
      << "[WasmParser::Implementation] [parseExpressionDecl] ENDED\n";
    return ExpressionDecl::create(getContext(), Instructions);
  }

#pragma mark Parsing Instructions

  InstNode parseInstruction(ReadContext& Ctx) {
    Instruction Opcode = Instruction(readOpcode(Ctx));
    std::cout
      << "[WasmParser::Implementation] [parseInstruction] OPCODE = 0x"
      // Adding a unary + operator before the variable of any primitive
      // data type will give printable numerical value instead of ASCII
      // character(in case of char type).
      //
      // https://stackoverflow.com/a/31991844/1393062
      << std::hex << +(uint8_t)Opcode << "\n";
    switch (Opcode) {
#define INST(Id, Opcode0, ...)                                           \
  case Instruction::Id:                                                  \
    return parse##Id(Ctx);
#include <w2n/AST/Instructions.def>
    }
  }

  UnreachableStmt * parseUnreachable(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  BlockStmt * parseBlock(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  LoopStmt * parseLoop(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  IfStmt * parseIf(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  EndStmt * parseEnd(ReadContext& Ctx) {
    return EndStmt::create(getContext());
  }

  BrStmt * parseBr(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  BrIfStmt * parseBrIf(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  BrTableStmt * parseBrTable(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  ReturnStmt * parseReturn(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallExpr * parseCall(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallIndirectExpr * parseCallIndirect(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  DropExpr * parseDrop(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  LocalGetExpr * parseLocalGet(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  LocalSetExpr * parseLocalSet(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  GlobalGetExpr * parseGlobalGet(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  GlobalSetExpr * parseGlobalSet(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  LoadExpr * parseI32Load(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  LoadExpr * parseI32Load8u(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  StoreExpr * parseI32Store(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  IntegerConstExpr * parseI32Const(ReadContext& Ctx) {
    uint32_t BitPattern = readUint32(Ctx);
    return IntegerConstExpr::create(
      getContext(),
      llvm::APInt(32, BitPattern, true),
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Eqz(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallBuiltinExpr * parseI32Eq(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallBuiltinExpr * parseI32Ne(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallBuiltinExpr * parseI32Add(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallBuiltinExpr * parseI32Sub(ReadContext& Ctx) {
    w2n_unimplemented();
  }

  CallBuiltinExpr * parseI32And(ReadContext& Ctx) {
    w2n_unimplemented();
  }

#pragma mark Parsing Sections

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
    StringRef Filename = Parser->File.getFilename();
    Identifier ModuleName = getContext().getIdentifier(Filename);
    ModuleDecl * Mod =
      ModuleDecl::create(ModuleName, Parser->File.getASTContext());
    for (auto& EachSectionDecl : SectionDecls) {
      Mod->addSectionDecl(EachSectionDecl);
    }
    return Mod;
  }

  CustomSectionDecl * parseCustomSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    w2n_unimplemented();
  }

  TypeSectionDecl * parseTypeSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    uint32_t Count = readVaruint32(Ctx);
    NumTypes = Count;
    std::vector<FuncTypeDecl *> FuncTypeDecls;
    FuncTypeDecls.reserve(Count);
    while (Count-- != 0) {
      uint8_t Form = readUint8(Ctx);
      if (Form != llvm::wasm::WASM_TYPE_FUNC) {
        llvm_unreachable("invalid signature type");
      }
      FuncType * Ty = parseFuncType(Ctx);
      FuncTypeDecl * FuncTypeDecl =
        FuncTypeDecl::create(getContext(), Ty);
      FuncTypeDecls.push_back(std::move(FuncTypeDecl));
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("type section ended prematurely");
    }
    return TypeSectionDecl::create(getContext(), FuncTypeDecls);
  }

  ImportSectionDecl * parseImportSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    uint32_t Count = readVaruint32(Ctx);
    size_t NumTypes = ParsedTypeSectionDecl->getTypes().size();
    std::vector<ImportDecl *> Imports;
    Imports.reserve(Count);
    for (uint32_t I = 0; I < Count; I++) {
      ImportDecl * Import;
      Identifier Module = getContext().getIdentifier(readString(Ctx));
      Identifier Name = getContext().getIdentifier(readString(Ctx));
      uint8_t Kind = readUint8(Ctx);
      switch (Kind) {
      case llvm::wasm::WASM_EXTERNAL_FUNCTION: {
        NumImportedFunctions++;
        uint32_t SigIndex = readVaruint32(Ctx);
        if (SigIndex >= NumTypes) {
          llvm_unreachable("invalid function type");
        }
        Import =
          ImportFuncDecl::create(getContext(), Module, Name, SigIndex);
        break;
      }
      case llvm::wasm::WASM_EXTERNAL_TABLE: {
        NumImportedTables++;
        TableType * TableTy = parseTableType(Ctx);
        Import =
          ImportTableDecl::create(getContext(), Module, Name, TableTy);
        break;
      }
      case llvm::wasm::WASM_EXTERNAL_MEMORY: {
        MemoryType * Memory = parseMemoryType(Ctx);
        Import =
          ImportMemoryDecl::create(getContext(), Module, Name, Memory);
        break;
      }
      case llvm::wasm::WASM_EXTERNAL_GLOBAL: {
        NumImportedGlobals++;
        GlobalType * GlobalTy = parseGlobalType(Ctx);
        Import =
          ImportGlobalDecl::create(getContext(), Module, Name, GlobalTy);
        break;
      }
      default: llvm_unreachable("unexpected import kind");
      }
      Imports.push_back(Import);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("import section ended prematurely");
    }
    return ImportSectionDecl::create(getContext(), Imports);
  }

  FuncSectionDecl * parseFuncSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<uint32_t> Functions;
    Functions.reserve(Count);
    while ((Count--) != 0) {
      uint32_t F = readVaruint32(Ctx);
      Functions.push_back(F);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("function section ended prematurely");
    }
    return FuncSectionDecl::create(getContext(), Functions);
  }

  TableSectionDecl * parseTableSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    TableSection = SectionIdx;
    uint32_t Count = readVaruint32(Ctx);
    std::vector<TableDecl *> Tables;
    Tables.reserve(Count);
    while ((Count--) != 0) {
      TableDecl * Table = parseTableDecl(Ctx);
      Tables.push_back(Table);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("table section ended prematurely");
    }
    return TableSectionDecl::create(getContext(), Tables);
  }

  MemorySectionDecl * parseMemorySectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<MemoryDecl *> Mems;
    Mems.reserve(Count);
    while ((Count--) != 0) {
      MemoryDecl * Mem = parseMemoryDecl(Ctx);
      Mems.push_back(Mem);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("memory section ended prematurely");
    }
    return MemorySectionDecl::create(getContext(), Mems);
  }

  GlobalSectionDecl * parseGlobalSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    GlobalSection = SectionIdx;
    uint32_t Count = readVaruint32(Ctx);
    std::vector<GlobalDecl *> Globals;
    Globals.reserve(Count);
    while ((Count--) != 0) {
      GlobalDecl * Global = parseGlobalDecl(Ctx);
      Globals.push_back(Global);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("global section ended prematurely");
    }
    return GlobalSectionDecl::create(getContext(), Globals);
  }

  ExportSectionDecl * parseExportSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<ExportDecl *> Exports;
    Exports.reserve(Count);
    for (uint32_t I = 0; I < Count; I++) {
      ExportDecl * Ex = parseExportDecl(Ctx);
      Exports.push_back(Ex);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("export section ended prematurely");
    }
    return ExportSectionDecl::create(getContext(), Exports);
  }

  StartSectionDecl * parseStartSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    w2n_unimplemented();
  }

  ElementSectionDecl * parseElementSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    w2n_unimplemented();
  }

  CodeSectionDecl * parseCodeSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    CodeSection = SectionIdx;
    uint32_t FunctionCount = readVaruint32(Ctx);
    // FIXME: FunctionCount != Functions.size() -> "invalid fn count"

    std::vector<CodeDecl *> Codes;
    Codes.reserve(FunctionCount);

    for (uint32_t I = 0; I < FunctionCount; I++) {
      CodeDecl * Code = parseCodeDecl(Ctx);
      Codes.push_back(Code);
    }
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("code section ended prematurely");
    }
    return CodeSectionDecl::create(getContext(), Codes);
  }

  DataSectionDecl * parseDataSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    w2n_unimplemented();
  }

  DataCountSectionDecl * parseDataCountSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
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
  SectionDecl *
  parseSectionDecl(const WasmSection& Section, size_t SectionIdx) {
    ReadContext Ctx;
    Ctx.Start = Section.Content.data();
    Ctx.End = Ctx.Start + Section.Content.size();
    Ctx.Ptr = Ctx.Start;

    switch (Section.Type) {
#define DECL(Id, Parent)
#define SECTION_DECL(Id, _)                                              \
  case W2N_FORMAT_GET_SEC_TYPE(Id):                                      \
    Parsed##Id##Decl = parse##Id##Decl(Section, Ctx, SectionIdx);        \
    return Parsed##Id##Decl;
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
    size_t SectionIdx = 0;
    while (CurrentSect != SectEnd) {
      const auto& Section = WasmObject->getWasmSection(*CurrentSect);
      SectionDecl * SectionDecl = parseSectionDecl(Section, SectionIdx);
      if (SectionDecl != nullptr) {
        Sections.push_back(SectionDecl);
      }
      CurrentSect = CurrentSect.operator++();
      SectionIdx++;
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
  StringRef Filename = SF.getFilename();
  StringRef Contents =
    SourceMgr.extractText(SourceMgr.getRangeForBuffer(BufferID));
  auto Object = llvm::MemoryBufferRef(Contents, Filename);
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
