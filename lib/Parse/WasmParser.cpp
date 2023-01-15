#include <_types/_uint32_t.h>
#include <_types/_uint8_t.h>
#include "llvm/ADT/None.h"
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Builtins.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Expr.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/Instructions.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/NameAssociation.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/Stmt.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/Compiler.h>
#include <w2n/Basic/ImplicitTrailingObject.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/Parse/Parser.h>

using namespace w2n;
using namespace llvm::object;

/// Current implementation of \c WasmParser is ripped from LLVM's
/// implementation of WebAssembly object file support. There are a lot of
/// room to improve the performance and the organization.

namespace w2n {

using TypeIndexTy = uint32_t;
using FuncIndexTy = uint32_t;
using TableIndexTy = uint32_t;
using MemIndexTy = uint32_t;
using GlobalIndexTy = uint32_t;
using ElemIndexTy = uint32_t;
using DataIndexTy = uint32_t;
using LocalIndexTy = uint32_t;
using LabelIndexTy = uint32_t;

enum class TypeKindImmediate {
  I32 = llvm::wasm::WASM_TYPE_I32,
  I64 = llvm::wasm::WASM_TYPE_I64,
  F32 = llvm::wasm::WASM_TYPE_F32,
  F64 = llvm::wasm::WASM_TYPE_F64,
  V128 = llvm::wasm::WASM_TYPE_V128,
  FuncRef = llvm::wasm::WASM_TYPE_FUNCREF,
  ExternRef = llvm::wasm::WASM_TYPE_EXTERNREF,
  Func = llvm::wasm::WASM_TYPE_FUNC,
  Void = llvm::wasm::WASM_TYPE_NORESULT,
};

enum class ExternalKindImmediate : uint8_t {
  Func = llvm::wasm::WASM_EXTERNAL_FUNCTION,
  Table = llvm::wasm::WASM_EXTERNAL_TABLE,
  Memory = llvm::wasm::WASM_EXTERNAL_MEMORY,
  Global = llvm::wasm::WASM_EXTERNAL_GLOBAL,
  Tag = llvm::wasm::WASM_EXTERNAL_TAG,
};

enum class SectionKindImmediate {
  CustomSection = llvm::wasm::WASM_SEC_CUSTOM,
  TypeSection = llvm::wasm::WASM_SEC_TYPE,
  ImportSection = llvm::wasm::WASM_SEC_IMPORT,
  FuncSection = llvm::wasm::WASM_SEC_FUNCTION,
  TableSection = llvm::wasm::WASM_SEC_TABLE,
  MemorySection = llvm::wasm::WASM_SEC_MEMORY,
  GlobalSection = llvm::wasm::WASM_SEC_GLOBAL,
  ExportSection = llvm::wasm::WASM_SEC_EXPORT,
  StartSection = llvm::wasm::WASM_SEC_START,
  ElementSection = llvm::wasm::WASM_SEC_ELEM,
  CodeSection = llvm::wasm::WASM_SEC_CODE,
  DataSection = llvm::wasm::WASM_SEC_DATA,
  DataCountSection = llvm::wasm::WASM_SEC_DATACOUNT,
};

enum class SubSectionKindImmediate : uint8_t {
  ModuleNames = 0,
  FuncNames = 1,
  LocalNames = 2,
};

enum class DataKindImmediate : uint32_t {
  ActiveZerothMemory = 0,
  Passive = 1,
  ActiveArbitraryMemory = 2,
};

static ValueTypeKind getValueTypeKind(TypeKindImmediate Ty) {
  switch (Ty) {
  case TypeKindImmediate::I32: return ValueTypeKind::I32;
  case TypeKindImmediate::I64: return ValueTypeKind::I64;
  case TypeKindImmediate::F32: return ValueTypeKind::F32;
  case TypeKindImmediate::F64: return ValueTypeKind::F64;
  case TypeKindImmediate::V128: return ValueTypeKind::V128;
  case TypeKindImmediate::FuncRef: return ValueTypeKind::FuncRef;
  case TypeKindImmediate::ExternRef: return ValueTypeKind::ExternRef;
  default: return ValueTypeKind::None;
  }
}

} // namespace w2n

#define W2N_VARIN_T7_MAX  ((1 << 7) - 1)
#define W2N_VARIN_T7_MIN  (-(1 << 7))
#define W2N_VARUIN_T7_MAX (1 << 7)
#define W2N_VARUIN_T1_MAX (1)

struct ReadContext {
  const uint8_t * Start;
  const uint8_t * Ptr;
  const uint8_t * End;
  llvm::Optional<uint32_t> ElementIndex;
};

static uint8_t readUint8(ReadContext& Ctx) {
  if (Ctx.Ptr == Ctx.End) {
    llvm_unreachable("EOF while reading uint8");
  }
  return *Ctx.Ptr++;
}

W2N_UNUSED

static uint32_t readUint32(ReadContext& Ctx) {
  if (Ctx.Ptr + 4 > Ctx.End) {
    llvm_unreachable("EOF while reading uint32");
  }
  uint32_t Result = llvm::support::endian::read32le(Ctx.Ptr);
  Ctx.Ptr += 4;
  return Result;
}

W2N_UNUSED

static int32_t readFloat32(ReadContext& Ctx) {
  if (Ctx.Ptr + 4 > Ctx.End) {
    llvm_unreachable("EOF while reading float64");
  }
  int32_t Result = 0;
  memcpy(&Result, Ctx.Ptr, sizeof(Result));
  Ctx.Ptr += sizeof(Result);
  return Result;
}

W2N_UNUSED

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

W2N_UNUSED

static int64_t readVarint64(ReadContext& Ctx) {
  return readLEB128(Ctx);
}

static uint64_t readVaruint64(ReadContext& Ctx) {
  return readULEB128(Ctx);
}

static uint8_t readOpcode(ReadContext& Ctx) {
  return readUint8(Ctx);
}

class WasmParser::Implementation {
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
  uint32_t BlockLevel = 0;

  explicit Implementation(WasmParser * Parser) : Parser(Parser) {
  }

  ASTContext& getContext() {
    return const_cast<const SourceFile&>(Parser->File).getASTContext();
  }

  const ASTContext& getContext() const {
    return Parser->File.getASTContext();
  }

  /// Basic templated parsing function.
  template <typename T>
  T parse(ReadContext& Ctx);

#pragma mark Parsing Basic Types

  // Merging parseVector into parse series funciton needs function partial
  // specialization which is not supported in C++.
  // FIXME: Refactor parse/parseVector series function with functor.
  template <typename T>
  std::vector<T> parseVector(ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<T> Vector;
    Vector.reserve(Count);
    for (uint32_t I = 0; I < Count; I++) {
      Ctx.ElementIndex = I;
      T Value = parse<T>(Ctx);
      Vector.push_back(Value);
    }
    Ctx.ElementIndex = llvm::None;
    return Vector;
  }

  template <>
  uint32_t parse<uint32_t>(ReadContext& Ctx) {
    return readVaruint32(Ctx);
  }

  template <>
  uint8_t parse<uint8_t>(ReadContext& Ctx) {
    return readUint8(Ctx);
  }

  template <>
  Identifier parse<Identifier>(ReadContext& Ctx) {
    return getContext().getIdentifier(readString(Ctx));
  }

#pragma mark Parsing Types

  template <>
  TypeKindImmediate parse<TypeKindImmediate>(ReadContext& Ctx) {
    uint32_t RawKind = readUint8(Ctx);
    return (TypeKindImmediate)RawKind;
  }

  template <>
  ValueType * parse<ValueType *>(ReadContext& Ctx) {
    TypeKindImmediate TyImm = parse<TypeKindImmediate>(Ctx);
    ValueTypeKind Kind = getValueTypeKind(TyImm);
    ValueType * Ty = getContext().getValueTypeForKind(Kind);
    assert(Ty);
    return Ty;
  }

  template <>
  LimitsType * parse<LimitsType *>(ReadContext& Ctx) {
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

  template <>
  TableType * parse<TableType *>(ReadContext& Ctx) {
    ValueType * ElemType = parse<ValueType *>(Ctx);
    ReferenceType * ElementType = dyn_cast<ReferenceType>(ElemType);
    if (ElementType == nullptr) {
      llvm_unreachable("invalid table element type");
    }
    LimitsType * Limits = parse<LimitsType *>(Ctx);
    return getContext().getTableType(ElementType, Limits);
  }

  template <>
  MemoryType * parse<MemoryType *>(ReadContext& Ctx) {
    LimitsType * Limits = parse<LimitsType *>(Ctx);
    return getContext().getMemoryType(Limits);
  }

  template <>
  GlobalType * parse<GlobalType *>(ReadContext& Ctx) {
    ValueType * Type = parse<ValueType *>(Ctx);
    bool IsMutable = readVaruint1(Ctx) != 0;
    return getContext().getGlobalType(Type, IsMutable);
  }

  template <>
  ResultType * parse<ResultType *>(ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    std::vector<ValueType *> ValueTypes;
    ValueTypes.reserve(Count);
    while (Count-- != 0) {
      ValueType * Type = parse<ValueType *>(Ctx);
      ValueTypes.push_back(Type);
    }
    return getContext().getResultType(ValueTypes);
  }

  template <>
  FuncType * parse<FuncType *>(ReadContext& Ctx) {
    ResultType * Params = parse<ResultType *>(Ctx);
    ResultType * Returns = parse<ResultType *>(Ctx);
    return getContext().getFuncType(Params, Returns);
  }

  template <>
  TypeIndexType * parse<TypeIndexType *>(ReadContext& Ctx) {
    uint32_t TypeIndex = readVarint32(Ctx);
    return getContext().getTypeIndexType(TypeIndex);
  }

  template <>
  BlockType * parse<BlockType *>(ReadContext& Ctx) {
    ReadContext ReservedCtx = Ctx;
    TypeKindImmediate TyImm = parse<TypeKindImmediate>(Ctx);
    if (TyImm == TypeKindImmediate::Void) {
      return BlockType::create(getContext(), getContext().getVoidType());
    }
    ValueTypeKind Kind = getValueTypeKind(TyImm);
    ValueType * ValTy = getContext().getValueTypeForKind(Kind);
    if (ValTy != nullptr) {
      return BlockType::create(getContext(), ValTy);
    }
    Ctx = ReservedCtx;
    TypeIndexType * TypeIndexTy = parse<TypeIndexType *>(Ctx);
    return BlockType::create(getContext(), TypeIndexTy);
  }

#pragma mark Parsing Variant Name Associations

  template <>
  NameAssociation parse<NameAssociation>(ReadContext& Ctx) {
    uint32_t Index = parse<uint32_t>(Ctx);
    Identifier Name = parse<Identifier>(Ctx);
    return {Index, Name};
  }

  template <>
  IndirectNameAssociation parse<IndirectNameAssociation>(ReadContext& Ctx
  ) {
    uint32_t Index = parse<uint32_t>(Ctx);
    std::vector<NameAssociation> NameMap =
      parseVector<NameAssociation>(Ctx);
    return {Index, NameMap};
  }

#pragma mark Parsing Direct Section Contents

  template <>
  ImportDecl * parse<ImportDecl *>(ReadContext& Ctx) {
    Identifier Module = getContext().getIdentifier(readString(Ctx));
    Identifier Name = getContext().getIdentifier(readString(Ctx));
    uint8_t RawKind = readUint8(Ctx);
    switch ((ExternalKindImmediate)RawKind) {
    case ExternalKindImmediate::Func: {
      NumImportedFunctions++;
      uint32_t SigIndex = readVaruint32(Ctx);
      if (SigIndex >= NumTypes) {
        llvm_unreachable("invalid function type");
      }
      return ImportFuncDecl::create(getContext(), Module, Name, SigIndex);
    }
    case ExternalKindImmediate::Table: {
      NumImportedTables++;
      TableType * TableTy = parse<TableType *>(Ctx);
      return ImportTableDecl::create(getContext(), Module, Name, TableTy);
    }
    case ExternalKindImmediate::Memory: {
      MemoryType * Memory = parse<MemoryType *>(Ctx);
      return ImportMemoryDecl::create(getContext(), Module, Name, Memory);
    }
    case ExternalKindImmediate::Global: {
      NumImportedGlobals++;
      GlobalType * GlobalTy = parse<GlobalType *>(Ctx);
      return ImportGlobalDecl::create(
        getContext(), Module, Name, GlobalTy
      );
    }
    case ExternalKindImmediate::Tag:
      llvm_unreachable("unexpected import kind");
    }
    llvm_unreachable("unexpected import kind");
  }

  template <>
  TableDecl * parse<TableDecl *>(ReadContext& Ctx) {
    TableType * Ty = parse<TableType *>(Ctx);
    return TableDecl::create(getContext(), Ty);
  }

  template <>
  MemoryDecl * parse<MemoryDecl *>(ReadContext& Ctx) {
    MemoryType * Ty = parse<MemoryType *>(Ctx);
    return MemoryDecl::create(getContext(), Ty);
  }

  template <>
  GlobalDecl * parse<GlobalDecl *>(ReadContext& Ctx) {
    GlobalType * Ty = parse<GlobalType *>(Ctx);
    ExpressionDecl * Init = parse<ExpressionDecl *>(Ctx);
    return GlobalDecl::create(
      getContext(), Ctx.ElementIndex.value(), Ty, Init
    );
  }

  template <>
  ExportDecl * parse<ExportDecl *>(ReadContext& Ctx) {
    Identifier Name = getContext().getIdentifier(readString(Ctx));
    uint8_t Kind = readUint8(Ctx);
    uint32_t Index = readVaruint32(Ctx);
    switch ((ExternalKindImmediate)Kind) {
    case ExternalKindImmediate::Func:
      // FIXME: !isDefinedFunctionIndex(Index) -> invalid function export
      return ExportFuncDecl::create(getContext(), Name, Index);
    case ExternalKindImmediate::Global:
      // FIXME: !isValidGlobalIndex(Index) -> invalid global export
      return ExportGlobalDecl::create(getContext(), Name, Index);
    case ExternalKindImmediate::Memory:
      return ExportMemoryDecl::create(getContext(), Name, Index);
    case ExternalKindImmediate::Table:
      return ExportTableDecl::create(getContext(), Name, Index);
    case ExternalKindImmediate::Tag:
      llvm_unreachable("tag is not supported");
    }
    llvm_unreachable("unexpected export kind");
  }

  template <>
  CodeDecl * parse<CodeDecl *>(ReadContext& Ctx) {
    uint32_t Size = readVaruint32(Ctx);
    FuncDecl * Func = parse<FuncDecl *>(Ctx);
    return CodeDecl::create(getContext(), Size, Func);
  }

  template <>
  FuncDecl * parse<FuncDecl *>(ReadContext& Ctx) {
    uint32_t NumLocalDecls = readVaruint32(Ctx);
    std::vector<LocalDecl *> Locals;
    Locals.reserve(NumLocalDecls);
    while ((NumLocalDecls--) != 0) {
      LocalDecl * Local = parse<LocalDecl *>(Ctx);
      Locals.push_back(Local);
    }
    ExpressionDecl * Expression = parse<ExpressionDecl *>(Ctx);
    // FIXME: validation
    return FuncDecl::create(getContext(), Locals, Expression);
  }

  template <>
  LocalDecl * parse<LocalDecl *>(ReadContext& Ctx) {
    uint32_t Count = readVaruint32(Ctx);
    ValueType * Type = parse<ValueType *>(Ctx);
    return LocalDecl::create(getContext(), Count, Type);
  }

  template <>
  ExpressionDecl * parse<ExpressionDecl *>(ReadContext& Ctx) {
    std::vector<InstNode> Instructions = parseInstructions(Ctx);
    return ExpressionDecl::create(getContext(), Instructions);
  }

  template <>
  DataSegmentDecl * parse<DataSegmentDecl *>(ReadContext& Ctx) {
    uint32_t RawKind = readVaruint32(Ctx);
    assert(RawKind >= 0 && RawKind <= 2);
    DataKindImmediate Kind = (DataKindImmediate)RawKind;
    switch (Kind) {
    case DataKindImmediate::ActiveZerothMemory: {
      ExpressionDecl * Expression = parse<ExpressionDecl *>(Ctx);
      std::vector<uint8_t> Data = parseVector<uint8_t>(Ctx);
      return DataSegmentActiveDecl::create(
        getContext(), 0, Expression, Data
      );
    }
    case DataKindImmediate::Passive: {
      std::vector<uint8_t> Data = parseVector<uint8_t>(Ctx);
      return DataSegmentPassiveDecl::create(getContext(), Data);
    }
    case DataKindImmediate::ActiveArbitraryMemory: {
      MemIndexTy MemoryIndex = parse<MemIndexTy>(Ctx);
      ExpressionDecl * Expression = parse<ExpressionDecl *>(Ctx);
      std::vector<uint8_t> Data = parseVector<uint8_t>(Ctx);
      return DataSegmentActiveDecl::create(
        getContext(), MemoryIndex, Expression, Data
      );
    }
    }
  }

  template <>
  SubSectionKindImmediate parse<SubSectionKindImmediate>(ReadContext& Ctx
  ) {
    uint8_t RawSubSectionId = parse<uint8_t>(Ctx);
    assert(RawSubSectionId >= 0 && RawSubSectionId <= 2);
    SubSectionKindImmediate SubSectionId =
      (SubSectionKindImmediate)RawSubSectionId;
    return SubSectionId;
  }

  template <>
  NameSubsectionDecl * parse<NameSubsectionDecl *>(ReadContext& Ctx) {
    SubSectionKindImmediate Kind = parse<SubSectionKindImmediate>(Ctx);
    switch (Kind) {
    case SubSectionKindImmediate::ModuleNames:
      return parse<ModuleNameSubsectionDecl *>(Ctx);
    case SubSectionKindImmediate::FuncNames:
      return parse<FuncNameSubsectionDecl *>(Ctx);
    case SubSectionKindImmediate::LocalNames:
      return parse<LocalNameSubsectionDecl *>(Ctx);
    }
  }

  template <>
  ModuleNameSubsectionDecl *
  parse<ModuleNameSubsectionDecl *>(ReadContext& Ctx) {
    W2N_UNUSED
    uint32_t Size = parse<uint32_t>(Ctx);
    std::vector<Identifier> Names = parseVector<Identifier>(Ctx);
    return ModuleNameSubsectionDecl::create(getContext(), Names);
  }

  template <>
  FuncNameSubsectionDecl *
  parse<FuncNameSubsectionDecl *>(ReadContext& Ctx) {
    W2N_UNUSED
    uint32_t Size = parse<uint32_t>(Ctx);
    std::vector<NameAssociation> NameMap =
      parseVector<NameAssociation>(Ctx);
    return FuncNameSubsectionDecl::create(getContext(), NameMap);
  }

  template <>
  LocalNameSubsectionDecl *
  parse<LocalNameSubsectionDecl *>(ReadContext& Ctx) {
    W2N_UNUSED
    uint32_t Size = parse<uint32_t>(Ctx);
    std::vector<IndirectNameAssociation> IndirectNameMap =
      parseVector<IndirectNameAssociation>(Ctx);
    return LocalNameSubsectionDecl::create(getContext(), IndirectNameMap);
  }

#pragma mark Parsing Instructions

  std::vector<InstNode> parseInstructions(ReadContext& Ctx) {
    std::vector<InstNode> Instructions;
    InstNode LastInstruction;
    std::tie(Instructions, LastInstruction) =
      parseInstructionsUntil(Ctx, [](InstNode Instruction) -> bool {
        return (EndStmt::classof(Instruction.dyn_cast<Stmt *>()));
      });
    Instructions.push_back(LastInstruction);
    return Instructions;
  }

  std::pair<std::vector<InstNode>, InstNode> parseInstructionsUntil(
    ReadContext& Ctx, std::function<bool(InstNode)> Predicate
  ) {
    BlockLevel++;
    std::vector<InstNode> Instructions;
    InstNode Instruction = nullptr;
    std::cout << "[WasmParser::Implementation] [parseInstructionsUntil] ["
              << BlockLevel << "] BEGAN\n";
    while (Instruction.isNull() || !Predicate(Instruction)) {
      Instruction = parseInstruction(Ctx);
      if (!Predicate(Instruction)) {
        Instructions.push_back(Instruction);
      }
    }
    std::cout << "[WasmParser::Implementation] [parseInstructionsUntil] ["
              << BlockLevel << "] ENDED\n";
    BlockLevel--;
    return std::make_pair(Instructions, Instruction);
  }

  InstNode parseInstruction(ReadContext& Ctx) {
    Instruction Opcode = Instruction(readOpcode(Ctx));
    std::cout << "[WasmParser::Implementation] [parseInstruction] ["
              << BlockLevel
              << "] OPCODE = 0x"
              // Adding a unary + operator before the variable of any
              // primitive data type will give printable numerical value
              // instead of ASCII character(in case of char type).
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
    return UnreachableStmt::create(getContext());
  }

  BlockStmt * parseBlock(ReadContext& Ctx) {
    BlockType * Ty = parse<BlockType *>(Ctx);
    std::vector<InstNode> Instructions;
    InstNode EndInstruction;
    std::tie(Instructions, EndInstruction) =
      parseInstructionsUntil(Ctx, [](InstNode Instruction) -> bool {
        return EndStmt::classof(Instruction.dyn_cast<Stmt *>());
      });
    return BlockStmt::create(
      getContext(),
      Ty,
      Instructions,
      dyn_cast<EndStmt>(EndInstruction.dyn_cast<Stmt *>())
    );
  }

  LoopStmt * parseLoop(ReadContext& Ctx) {
    BlockType * Ty = parse<BlockType *>(Ctx);
    std::vector<InstNode> Instructions;
    InstNode EndInstruction;
    std::tie(Instructions, EndInstruction) =
      parseInstructionsUntil(Ctx, [](InstNode Instruction) -> bool {
        return EndStmt::classof(Instruction.dyn_cast<Stmt *>());
      });
    return LoopStmt::create(
      getContext(),
      Ty,
      Instructions,
      dyn_cast<EndStmt>(EndInstruction.dyn_cast<Stmt *>())
    );
  }

  IfStmt * parseIf(ReadContext& Ctx) {
    BlockType * Ty = parse<BlockType *>(Ctx);
    std::vector<InstNode> TrueInstructions;
    InstNode IntermediateInstruction;
    std::tie(TrueInstructions, IntermediateInstruction) =
      parseInstructionsUntil(Ctx, [](InstNode Instruction) -> bool {
        return EndStmt::classof(Instruction.dyn_cast<Stmt *>())
            || ElseStmt::classof(Instruction.dyn_cast<Stmt *>());
      });

    if (IntermediateInstruction.isStmt(StmtKind::End)) {
      return IfStmt::create(
        getContext(),
        Ty,
        TrueInstructions,
        nullptr,
        llvm::None,
        dyn_cast<EndStmt>(IntermediateInstruction.dyn_cast<Stmt *>())
      );
    }

    if (IntermediateInstruction.isStmt(StmtKind::Else)) {
      std::vector<InstNode> FalseInstructions;
      InstNode EndInstruction;
      std::tie(FalseInstructions, EndInstruction) =
        parseInstructionsUntil(Ctx, [](InstNode Instruction) -> bool {
          return EndStmt::classof(Instruction.dyn_cast<Stmt *>());
        });
      return IfStmt::create(
        getContext(),
        Ty,
        TrueInstructions,
        dyn_cast<ElseStmt>(IntermediateInstruction.dyn_cast<Stmt *>()),
        FalseInstructions,
        dyn_cast<EndStmt>(EndInstruction.dyn_cast<Stmt *>())
      );
    }

    llvm_unreachable("unexpected StmtKind");
  }

  EndStmt * parseEnd(ReadContext& Ctx) {
    return EndStmt::create(getContext());
  }

  BrStmt * parseBr(ReadContext& Ctx) {
    LabelIndexTy LabelIndex = parse<LabelIndexTy>(Ctx);
    return BrStmt::create(getContext(), LabelIndex);
  }

  BrIfStmt * parseBrIf(ReadContext& Ctx) {
    LabelIndexTy LabelIndex = parse<LabelIndexTy>(Ctx);
    return BrIfStmt::create(getContext(), LabelIndex);
  }

  BrTableStmt * parseBrTable(ReadContext& Ctx) {
    std::vector<LabelIndexTy> LabelIndices =
      parseVector<LabelIndexTy>(Ctx);
    LabelIndexTy DefaultLabelIndex = parse<LabelIndexTy>(Ctx);
    return BrTableStmt::create(
      getContext(), LabelIndices, DefaultLabelIndex
    );
  }

  ReturnStmt * parseReturn(ReadContext& Ctx) {
    return ReturnStmt::create(getContext());
  }

  CallExpr * parseCall(ReadContext& Ctx) {
    FuncIndexTy FuncIndex = parse<FuncIndexTy>(Ctx);
    return CallExpr::create(getContext(), FuncIndex);
  }

  CallIndirectExpr * parseCallIndirect(ReadContext& Ctx) {
    TypeIndexTy TypeIndex = parse<TypeIndexTy>(Ctx);
    TableIndexTy TableIndex = parse<TableIndexTy>(Ctx);
    return CallIndirectExpr::create(getContext(), TypeIndex, TableIndex);
  }

  DropExpr * parseDrop(ReadContext& Ctx) {
    return DropExpr::create(getContext());
  }

  LocalGetExpr * parseLocalGet(ReadContext& Ctx) {
    LocalIndexTy LocalIndex = parse<LocalIndexTy>(Ctx);
    return LocalGetExpr::create(getContext(), LocalIndex);
  }

  LocalSetExpr * parseLocalSet(ReadContext& Ctx) {
    LocalIndexTy LocalIndex = parse<LocalIndexTy>(Ctx);
    return LocalSetExpr::create(getContext(), LocalIndex);
  }

  GlobalGetExpr * parseGlobalGet(ReadContext& Ctx) {
    GlobalIndexTy GlobalIndex = parse<GlobalIndexTy>(Ctx);
    return GlobalGetExpr::create(getContext(), GlobalIndex);
  }

  GlobalSetExpr * parseGlobalSet(ReadContext& Ctx) {
    GlobalIndexTy GlobalIndex = parse<GlobalIndexTy>(Ctx);
    return GlobalSetExpr::create(getContext(), GlobalIndex);
  }

  MemoryArgument parseMemArg(ReadContext& Ctx) {
    uint32_t Align = readVaruint32(Ctx);
    uint32_t Offset = readVaruint32(Ctx);
    return MemoryArgument{Align, Offset};
  }

  LoadExpr * parseI32Load(ReadContext& Ctx) {
    MemoryArgument MemArg = parseMemArg(Ctx);
    return LoadExpr::create(
      getContext(),
      MemArg,
      getContext().getI32Type(),
      getContext().getI32Type()
    );
  }

  LoadExpr * parseI32Load8u(ReadContext& Ctx) {
    MemoryArgument MemArg = parseMemArg(Ctx);
    return LoadExpr::create(
      getContext(),
      MemArg,
      getContext().getU8Type(),
      getContext().getI32Type()
    );
  }

  StoreExpr * parseI32Store(ReadContext& Ctx) {
    MemoryArgument MemArg = parseMemArg(Ctx);
    return StoreExpr::create(
      getContext(),
      MemArg,
      getContext().getI32Type(),
      getContext().getI32Type()
    );
  }

  IntegerConstExpr * parseI32Const(ReadContext& Ctx) {
    uint32_t BitPattern = readVaruint32(Ctx);
    return IntegerConstExpr::create(
      getContext(),
      llvm::APInt(32, BitPattern, true),
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Eqz(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::ICMP_EQZ);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      /// FIXME: May need \c BooleanType ?
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Eq(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::ICMP_EQ);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      /// FIXME: May need \c BooleanType ?
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Ne(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::ICMP_NE);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      /// FIXME: May need \c BooleanType ?
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Add(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::Add);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32Sub(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::Sub);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      getContext().getI32Type()
    );
  }

  CallBuiltinExpr * parseI32And(ReadContext& Ctx) {
    StringRef BuiltinName = getBuiltinName(BuiltinValueKind::And);
    return CallBuiltinExpr::create(
      getContext(),
      getContext().getIdentifier(BuiltinName),
      getContext().getI32Type()
    );
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
    if (Section.Name == "name") {
      return parseNameSectionDecl(Section, Ctx, SectionIdx);
    }
    llvm_unreachable("unexpected custom section name.");
  }

  NameSectionDecl * parseNameSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    if (Ctx.Ptr == Ctx.End) {
      return NameSectionDecl::create(
        getContext(), nullptr, nullptr, nullptr
      );
    }

    struct SubsectionStorageRAII {
      ModuleNameSubsectionDecl * ModuleNames;
      FuncNameSubsectionDecl * FuncNames;
      LocalNameSubsectionDecl * LocalNames;

      void add(NameSubsectionDecl * NameSubsection) {
        ModuleNameSubsectionDecl * ModuleNames =
          dyn_cast<ModuleNameSubsectionDecl>(NameSubsection);
        if (ModuleNames != nullptr) {
          this->ModuleNames = ModuleNames;
          return;
        }

        FuncNameSubsectionDecl * FuncNames =
          dyn_cast<FuncNameSubsectionDecl>(NameSubsection);
        if (FuncNames != nullptr) {
          this->FuncNames = FuncNames;
          return;
        }

        LocalNameSubsectionDecl * LocalNames =
          dyn_cast<LocalNameSubsectionDecl>(NameSubsection);
        if (LocalNames != nullptr) {
          this->LocalNames = LocalNames;
          return;
        }
      }
    };

    SubsectionStorageRAII Store = SubsectionStorageRAII();

    NameSubsectionDecl * NameSubsection0 =
      parse<NameSubsectionDecl *>(Ctx);
    Store.add(NameSubsection0);

    if (Ctx.Ptr == Ctx.End) {
      return NameSectionDecl::create(
        getContext(), Store.ModuleNames, Store.FuncNames, Store.LocalNames
      );
    }

    NameSubsectionDecl * NameSubsection1 =
      parse<NameSubsectionDecl *>(Ctx);
    Store.add(NameSubsection1);

    if (Ctx.Ptr == Ctx.End) {
      return NameSectionDecl::create(
        getContext(), Store.ModuleNames, Store.FuncNames, Store.LocalNames
      );
    }

    NameSubsectionDecl * NameSubsection2 =
      parse<NameSubsectionDecl *>(Ctx);
    Store.add(NameSubsection2);

    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("custom section name ended prematurely");
    }

    return NameSectionDecl::create(
      getContext(), Store.ModuleNames, Store.FuncNames, Store.LocalNames
    );
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
      if ((TypeKindImmediate)Form != TypeKindImmediate::Func) {
        llvm_unreachable("invalid signature type");
      }
      FuncType * Ty = parse<FuncType *>(Ctx);
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
    std::vector<ImportDecl *> Imports = parseVector<ImportDecl *>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("import section ended prematurely");
    }
    return ImportSectionDecl::create(getContext(), Imports);
  }

  FuncSectionDecl * parseFuncSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    std::vector<uint32_t> Functions = parseVector<uint32_t>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("function section ended prematurely");
    }
    return FuncSectionDecl::create(getContext(), Functions);
  }

  TableSectionDecl * parseTableSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    TableSection = SectionIdx;
    std::vector<TableDecl *> Tables = parseVector<TableDecl *>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("table section ended prematurely");
    }
    return TableSectionDecl::create(getContext(), Tables);
  }

  MemorySectionDecl * parseMemorySectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    std::vector<MemoryDecl *> Mems = parseVector<MemoryDecl *>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("memory section ended prematurely");
    }
    return MemorySectionDecl::create(getContext(), Mems);
  }

  GlobalSectionDecl * parseGlobalSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    GlobalSection = SectionIdx;
    std::vector<GlobalDecl *> Globals = parseVector<GlobalDecl *>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("global section ended prematurely");
    }
    return GlobalSectionDecl::create(getContext(), Globals);
  }

  ExportSectionDecl * parseExportSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    std::vector<ExportDecl *> Exports = parseVector<ExportDecl *>(Ctx);
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
    std::vector<CodeDecl *> Codes = parseVector<CodeDecl *>(Ctx);
    // FIXME: FunctionCount != Functions.size() -> "invalid fn count"
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("code section ended prematurely");
    }
    return CodeSectionDecl::create(getContext(), Codes);
  }

  DataSectionDecl * parseDataSectionDecl(
    const WasmSection& Section, ReadContext& Ctx, size_t SectionIdx
  ) {
    DataSection = SectionIdx;
    /// FIXME: \c Validate vector count with DataCountSection's data;
    std::vector<DataSegmentDecl *> Data =
      parseVector<DataSegmentDecl *>(Ctx);
    if (Ctx.Ptr != Ctx.End) {
      llvm_unreachable("data section ended prematurely");
    }
    return DataSectionDecl::create(getContext(), Data);
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

    switch ((SectionKindImmediate)Section.Type) {
#define DECL(Id, Parent)
#define CUSTOM_SECTION_DECL(Id, Parent)
#define SECTION_DECL(Id, _)                                              \
  case SectionKindImmediate::Id:                                         \
    std::cout << "Parse " << #Id << "\n";                                \
    Parsed##Id##Decl = parse##Id##Decl(Section, Ctx, SectionIdx);        \
    return Parsed##Id##Decl;
#include <w2n/AST/DeclNodes.def>
    case SectionKindImmediate::CustomSection:
      std::cout << "Parse CustomSection\n";
      return parseCustomSectionDecl(Section, Ctx, SectionIdx);
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
