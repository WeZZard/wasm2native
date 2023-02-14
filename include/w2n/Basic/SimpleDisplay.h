/// This file defines the main customization point, \c simple_display, for
/// displaying values of a given type
/// encoding of (static) type information for use as a simple replacement
/// for run-time type information.

#ifndef W2N_BASIC_SIMPLEDISPLAY_H
#define W2N_BASIC_SIMPLEDISPLAY_H

#include <llvm/ADT/TinyPtrVector.h>
#include <llvm/Support/raw_ostream.h>
#include <tuple>
#include <type_traits>
#include <utility>

namespace w2n {
template <typename T>
struct HasTrivialDisplay {
  static const bool Value = false;
};

#define HAS_TRIVIAL_DISPLAY(Type)                                        \
  template <>                                                            \
  struct HasTrivialDisplay<Type> {                                       \
    static const bool Value = true;                                      \
  }

HAS_TRIVIAL_DISPLAY(unsigned char);
HAS_TRIVIAL_DISPLAY(signed char);
HAS_TRIVIAL_DISPLAY(char);
HAS_TRIVIAL_DISPLAY(short);
HAS_TRIVIAL_DISPLAY(unsigned short);
HAS_TRIVIAL_DISPLAY(int);
HAS_TRIVIAL_DISPLAY(unsigned int);
HAS_TRIVIAL_DISPLAY(long);
HAS_TRIVIAL_DISPLAY(unsigned long);
HAS_TRIVIAL_DISPLAY(long long);
HAS_TRIVIAL_DISPLAY(unsigned long long);
HAS_TRIVIAL_DISPLAY(float);
HAS_TRIVIAL_DISPLAY(double);
HAS_TRIVIAL_DISPLAY(bool);
HAS_TRIVIAL_DISPLAY(std::string);

#undef HAS_TRIVIAL_DISPLAY

template <typename T>
typename std::enable_if<HasTrivialDisplay<T>::Value>::type
simple_display(llvm::raw_ostream& os, const T& ss) {
  os << ss;
}

template <
  unsigned I,
  typename... Types,
  typename std::enable_if<I == sizeof...(Types)>::type * = nullptr>
void simple_display_tuple(
  llvm::raw_ostream& os, const std::tuple<Types...>& ss
);

template <
  unsigned I,
  typename... Types,
  typename std::enable_if<I<sizeof...(Types)>::type * = nullptr> void
    simple_display_tuple(
      llvm::raw_ostream& os, const std::tuple<Types...>& ss
    ) {
  // Start or separator.
  if (I == 0) {
    os << "(";
  } else {
    os << ", ";
  }

  // Current element.
  simple_display(os, std::get<I>(ss));

  // Print the remaining elements.
  simple_display_tuple<I + 1>(os, ss);
}

template <
  unsigned I,
  typename... Types,
  typename std::enable_if<I == sizeof...(Types)>::type *>
void simple_display_tuple(
  llvm::raw_ostream& os, const std::tuple<Types...>& ss
) {
  // Last element.
  os << ")";
}

template <typename... Types>
void simple_display(
  llvm::raw_ostream& os, const std::tuple<Types...>& ss
) {
  simple_display_tuple<0>(os, ss);
}

template <typename T1, typename T2>
void simple_display(llvm::raw_ostream& os, const std::pair<T1, T2>& ss) {
  os << "(";
  simple_display(os, ss.first);
  os << ", ";
  simple_display(os, ss.second);
  os << ")";
}

template <typename T>
void simple_display(
  llvm::raw_ostream& os, const llvm::TinyPtrVector<T>& ss
) {
  os << "{";
  bool First = true;
  for (const T& Value : ss) {
    if (First) {
      First = false;
    } else {
      os << ", ";
    }

    simple_display(os, Value);
  }
  os << "}";
}

template <typename T>
void simple_display(llvm::raw_ostream& os, const llvm::ArrayRef<T>& ss) {
  os << "{";
  bool First = true;
  for (const T& Value : ss) {
    if (First) {
      First = false;
    } else {
      os << ", ";
    }

    simple_display(os, Value);
  }
  os << "}";
}

template <typename T>
void simple_display(
  llvm::raw_ostream& os, const llvm::SmallVectorImpl<T>& ss
) {
  os << "{";
  bool First = true;
  for (const T& Value : ss) {
    if (First) {
      First = false;
    } else {
      os << ", ";
    }

    simple_display(os, Value);
  }
  os << "}";
}

template <typename T>
void simple_display(llvm::raw_ostream& os, const std::vector<T>& ss) {
  os << "{";
  bool First = true;
  for (const T& Value : ss) {
    if (First) {
      First = false;
    } else {
      os << ", ";
    }

    simple_display(os, Value);
  }
  os << "}";
}

template <typename T, typename U>
void simple_display(
  llvm::raw_ostream& os, const llvm::PointerUnion<T, U>& ss
) {
  if (const auto Subject = ss.template dyn_cast<T>()) {
    simple_display(os, Subject);
  } else {
    simple_display(os, ss.template get<U>());
  }
}
} // namespace w2n

namespace llvm {
template <typename T>
void simple_display(raw_ostream& os, const Optional<T>& ss) {
  if (ss) {
    simple_display(os, *ss);
  } else {
    os << "None";
  }
}
} // namespace llvm

#endif // W2N_BASIC_SIMPLEDISPLAY_H
