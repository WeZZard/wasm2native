#ifndef W2N_IRGEN_IRGEN
#define W2N_IRGEN_IRGEN

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Mutex.h>
#include <llvm/Target/TargetMachine.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/Basic/Statistic.h>
#include <w2n/TBDGen/TBDGen.h>

namespace w2n {

namespace irgen {
class IRGenFunction;

enum class StackProtectorMode : bool {
  NoStackProtector,
  StackProtector
};

class Size;

enum IsPOD_t : bool {
  IsNotPOD,
  IsPOD
};

inline IsPOD_t operator&(IsPOD_t l, IsPOD_t r) {
  return IsPOD_t(unsigned(l) & unsigned(r));
}

inline IsPOD_t& operator&=(IsPOD_t& l, IsPOD_t r) {
  return (l = (l & r));
}

enum IsFixedSize_t : bool {
  IsNotFixedSize,
  IsFixedSize
};

inline IsFixedSize_t operator&(IsFixedSize_t l, IsFixedSize_t r) {
  return IsFixedSize_t(unsigned(l) & unsigned(r));
}

inline IsFixedSize_t& operator&=(IsFixedSize_t& l, IsFixedSize_t r) {
  return (l = (l & r));
}

enum IsLoadable_t : bool {
  IsNotLoadable,
  IsLoadable
};

inline IsLoadable_t operator&(IsLoadable_t l, IsLoadable_t r) {
  return IsLoadable_t(unsigned(l) & unsigned(r));
}

inline IsLoadable_t& operator&=(IsLoadable_t& l, IsLoadable_t r) {
  return (l = (l & r));
}

enum IsBitwiseTakable_t : bool {
  IsNotBitwiseTakable,
  IsBitwiseTakable
};

inline IsBitwiseTakable_t
operator&(IsBitwiseTakable_t l, IsBitwiseTakable_t r) {
  return IsBitwiseTakable_t(unsigned(l) & unsigned(r));
}

inline IsBitwiseTakable_t&
operator&=(IsBitwiseTakable_t& l, IsBitwiseTakable_t r) {
  return (l = (l & r));
}

enum IsABIAccessible_t : bool {
  IsNotABIAccessible = false,
  IsABIAccessible = true
};

/// The atomicity of a reference counting operation to be used.
enum class Atomicity : bool {
  /// Atomic reference counting operations should be used.
  Atomic,
  /// Non-atomic reference counting operations can be used.
  NonAtomic,
};

/// Whether or not an object should be emitted on the heap.
enum OnHeap_t : unsigned char {
  NotOnHeap,
  OnHeap
};

/// Whether a function requires extra data.
enum class ExtraData : uint8_t {
  /// The function requires no extra data.
  None,

  /// The function requires a retainable object pointer of extra data.
  Retainable,

  /// The function takes its block object as extra data.
  Block,

  Last_ExtraData = Block
};

/// Given that we have metadata for a type, is it for exactly the
/// specified type, or might it be a subtype?
enum IsExact_t : bool {
  IsInexact = false,
  IsExact = true
};

/// Ways in which an object can be referenced.
///
/// See the comment in RelativePointer.h.

enum class SymbolReferenceKind : uint8_t {
  /// An absolute reference to the object, i.e. an ordinary pointer.
  ///
  /// Generally well-suited for when C compatibility is a must, dynamic
  /// initialization is the dominant case, or the runtime performance
  /// of accesses is an overriding concern.
  Absolute,

  /// A direct relative reference to the object, i.e. the offset of the
  /// object from the address at which the relative reference is stored.
  ///
  /// Generally well-suited for when the reference is always statically
  /// initialized and will always refer to another object within the
  /// same linkage unit.
  Relative_Direct,

  /// A direct relative reference that is guaranteed to be as wide as a
  /// pointer.
  ///
  /// Generally well-suited for when the reference may be dynamically
  /// initialized, but will only refer to objects within the linkage unit
  /// when statically initialized.
  Far_Relative_Direct,

  /// A relative reference that may be indirect: the direct reference is
  /// either directly to the object or to a variable holding an absolute
  /// reference to the object.
  ///
  /// The low bit of the target offset is used to mark an indirect
  /// reference, and so the low bit of the target address must be zero.
  /// This means that, in general, it is not possible to form this kind of
  /// reference to a function (due to the THUMB bit) or unaligned data
  /// (such as a C string).
  ///
  /// Generally well-suited for when the reference is always statically
  /// initialized but may refer to something outside of the linkage unit.
  Relative_Indirectable,

  /// An indirectable reference to the object; guaranteed to be as wide
  /// as a pointer.
  ///
  /// Generally well-suited for when the reference may be dynamically
  /// initialized but may also statically refer outside of the linkage
  /// unit.
  Far_Relative_Indirectable,
};

// The following 2 todos makes use fo clang codegen infra. Keep them todo.

// TODO: LazyConstantInitializer
// TODO: ConstantInit

/// An abstraction for computing the cost of an operation.
enum class OperationCost : unsigned {
  Free = 0,
  Arithmetic = 1,
  Load = 3, // TODO: split into static- and dynamic-offset cases?
  Call = 10
};

inline OperationCost operator+(OperationCost l, OperationCost r) {
  return OperationCost(unsigned(l) + unsigned(r));
}

inline OperationCost& operator+=(OperationCost& l, OperationCost r) {
  l = l + r;
  return l;
}

inline bool operator<(OperationCost l, OperationCost r) {
  return unsigned(l) < unsigned(r);
}

inline bool operator<=(OperationCost l, OperationCost r) {
  return unsigned(l) <= unsigned(r);
}

/// An alignment value, in eight-bit units.
class Alignment {
public:

  using int_type = uint64_t;

  constexpr Alignment() : Shift(0) {
  }

  explicit Alignment(int_type Value) : Shift(llvm::Log2_64(Value)) {
    assert(llvm::isPowerOf2_64(Value));
  }

  // TODO: explicit Alignment(clang::CharUnits value)

  constexpr int_type getValue() const {
    return int_type(1) << Shift;
  }

  constexpr int_type getMaskValue() const {
    return getValue() - 1;
  }

  Alignment alignmentAtOffset(Size S) const;
  Size asSize() const;

  unsigned log2() const {
    return Shift;
  }

  // TODO: operator clang::CharUnits() const

  // TODO: clang::CharUnits asCharUnits() const

  explicit operator llvm::MaybeAlign() const {
    return llvm::MaybeAlign(getValue());
  }

  friend bool operator<(Alignment L, Alignment R) {
    return L.Shift < R.Shift;
  }

  friend bool operator<=(Alignment L, Alignment R) {
    return L.Shift <= R.Shift;
  }

  friend bool operator>(Alignment L, Alignment R) {
    return L.Shift > R.Shift;
  }

  friend bool operator>=(Alignment L, Alignment R) {
    return L.Shift >= R.Shift;
  }

  friend bool operator==(Alignment L, Alignment R) {
    return L.Shift == R.Shift;
  }

  friend bool operator!=(Alignment L, Alignment R) {
    return L.Shift != R.Shift;
  }

  template <unsigned Value>
  static constexpr Alignment create() {
    Alignment result;
    result.Shift = llvm::CTLog2<Value>();
    return result;
  }

private:

  unsigned char Shift;
};

/// A size value, in eight-bit units.
class Size {
public:

  using int_type = uint64_t;

  constexpr Size() : Value(0) {
  }

  explicit constexpr Size(int_type Value) : Value(Value) {
  }

  static constexpr Size forBits(int_type bitSize) {
    return Size((bitSize + 7U) / 8U);
  }

  /// An "invalid" size, equal to the maximum possible size.
  static constexpr Size invalid() {
    return Size(~int_type(0));
  }

  /// Is this the "invalid" size value?
  bool isInvalid() const {
    return *this == Size::invalid();
  }

  constexpr int_type getValue() const {
    return Value;
  }

  int_type getValueInBits() const {
    return Value * 8;
  }

  bool isZero() const {
    return Value == 0;
  }

  friend Size operator+(Size L, Size R) {
    return Size(L.Value + R.Value);
  }

  friend Size& operator+=(Size& L, Size R) {
    L.Value += R.Value;
    return L;
  }

  friend Size operator-(Size L, Size R) {
    return Size(L.Value - R.Value);
  }

  friend Size& operator-=(Size& L, Size R) {
    L.Value -= R.Value;
    return L;
  }

  friend Size operator*(Size L, int_type R) {
    return Size(L.Value * R);
  }

  friend Size operator*(int_type L, Size R) {
    return Size(L * R.Value);
  }

  friend Size& operator*=(Size& L, int_type R) {
    L.Value *= R;
    return L;
  }

  friend int_type operator/(Size L, Size R) {
    return L.Value / R.Value;
  }

  explicit operator bool() const {
    return Value != 0;
  }

  Size roundUpToAlignment(Alignment align) const {
    int_type value = getValue() + align.getValue() - 1;
    return Size(value & ~int_type(align.getValue() - 1));
  }

  bool isPowerOf2() const {
    auto value = getValue();
    return ((value & -value) == value);
  }

  bool isMultipleOf(Size other) const {
    return (Value % other.Value) == 0;
  }

  unsigned log2() const {
    return llvm::Log2_64(Value);
  }

  // TODO: operator clang::CharUnits() const

  // TODO: clang::CharUnits asCharUnits() const

  friend bool operator<(Size L, Size R) {
    return L.Value < R.Value;
  }

  friend bool operator<=(Size L, Size R) {
    return L.Value <= R.Value;
  }

  friend bool operator>(Size L, Size R) {
    return L.Value > R.Value;
  }

  friend bool operator>=(Size L, Size R) {
    return L.Value >= R.Value;
  }

  friend bool operator==(Size L, Size R) {
    return L.Value == R.Value;
  }

  friend bool operator!=(Size L, Size R) {
    return L.Value != R.Value;
  }

  friend Size operator%(Size L, Alignment R) {
    return Size(L.Value % R.getValue());
  }

private:

  int_type Value;
};

/// Compute the alignment of a pointer which points S bytes after a
/// pointer with this alignment.
inline Alignment Alignment::alignmentAtOffset(Size S) const {
  assert(getValue() && "called on object with zero alignment");

  // If the offset is zero, use the original alignment.
  Size::int_type V = S.getValue();
  if (!V)
    return *this;

  // Find the offset's largest power-of-two factor.
  V = V & -V;

  // The alignment at the offset is then the min of the two values.
  if (V < getValue())
    return Alignment(static_cast<Alignment::int_type>(V));
  return *this;
}

/// Get this alignment as a Size value.
inline Size Alignment::asSize() const {
  return Size(getValue());
}

/// A static or dynamic offset.
class Offset {
  enum Kind {
    Static,
    Dynamic,
  };

  enum : uint64_t {
    KindBits = 1,
    KindMask = (1 << KindBits) - 1,
    PayloadMask = ~uint64_t(KindMask)
  };

  uint64_t Data;

public:

  explicit Offset(llvm::Value * offset) :
    Data(reinterpret_cast<uintptr_t>(offset) | Dynamic) {
  }

  explicit Offset(Size offset) :
    Data(
      (static_cast<uint64_t>(offset.getValue()) << KindBits) | Static
    ) {
    assert(getStatic() == offset && "overflow");
  }

  bool isStatic() const {
    return (Data & KindMask) == Static;
  }

  bool isDynamic() const {
    return (Data & KindMask) == Dynamic;
  }

  Size getStatic() const {
    assert(isStatic());
    return Size(static_cast<int64_t>(Data) >> KindBits);
  }

  llvm::Value * getDynamic() const {
    assert(isDynamic());
    return reinterpret_cast<llvm::Value *>(Data & PayloadMask);
  }

  llvm::Value * getAsValue(IRGenFunction& IGF) const;
  Offset offsetBy(IRGenFunction& IGF, Size other) const;
};

} // end namespace irgen

bool performLLVM(
  const IRGenOptions& Opts,
  DiagnosticEngine& Diags,
  llvm::sys::Mutex * DiagMutex,
  llvm::GlobalVariable * HashGlobal,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  StringRef OutputFilename,
  UnifiedStatsReporter * Stats
);

/// Given an already created LLVM module, construct a pass pipeline and
/// run the Swift LLVM Pipeline upon it. This does not cause the module to
/// be printed, only to be optimized.
void performLLVMOptimizations(
  const IRGenOptions& Opts,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine
);

/// Compiles and writes the given LLVM module into an output stream in the
/// format specified in the \c IRGenOptions.
bool compileAndWriteLLVM(
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  const IRGenOptions& Opts,
  UnifiedStatsReporter * Stats,
  DiagnosticEngine& Diags,
  llvm::raw_pwrite_stream& Out,
  llvm::sys::Mutex * DiagMutex = nullptr
);

/// Get the CPU, subtarget feature options, and triple to use when
/// emitting code.
std::tuple<
  llvm::TargetOptions,
  std::string,
  std::vector<std::string>,
  std::string>
getIRTargetOptions(const IRGenOptions& Opts, ASTContext& Ctx);

/// Creates a TargetMachine from the IRGen options and AST Context.
std::unique_ptr<llvm::TargetMachine>
createTargetMachine(const IRGenOptions& Opts, ASTContext& Ctx);

/// Turn the given Swift module into LLVM IR and return the generated
/// module. To compile and output the generated code, call \c performLLVM.
GeneratedModule performIRGeneration(
  ModuleDecl * M,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  ArrayRef<std::string> ParallelOutputFilenames,
  llvm::GlobalVariable ** OutModuleHash = nullptr
);

/// Turn the given Swift file into LLVM IR and return the generated
/// module. To compile and output the generated code, call \c performLLVM.
GeneratedModule performIRGeneration(
  FileUnit * File,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  llvm::GlobalVariable ** OutModuleHash = nullptr
);

} // namespace w2n

#endif // W2N_IRGEN_IRGEN
