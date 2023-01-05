#ifndef W2N_BASIC_LLVMRTTI_H
#define W2N_BASIC_LLVMRTTI_H

/**
 * @brief Declares the \c classof static function of LLVM-style RTTI for
 * root class \c ROOT_CLASS .
 *
 * @note This macros requires you to define the kind enum (result type of
 * \c getKind() function) to be named as \c FooKind if the class
 * represented by \c ROOT_CLASS is named as \c Foo . At the same time, it
 * also requires you to defines the last kind for drived classes in your
 * kind enum.
 *
 */
#define LLVM_RTTI_CLASSOF_ROOT_CLASS(ROOT_CLASS)                         \
  static bool classof(const ROOT_CLASS * I) {                            \
    if (I == nullptr) {                                                  \
      return false;                                                      \
    }                                                                    \
    return I->getKind() <= ROOT_CLASS##Kind::Last_##ROOT_CLASS;          \
  }

/**
 * @brief Declares the \c classof static function of LLVM-style RTTI for
 * non-leaf class of \c DERIVED_CLASS whose root class is \c ROOT_CLASS .
 *
 * @note This macros requires you to define the kind enum (result type of
 * \c getKind() function) to be named as \c FooKind if the class
 * represented by \c ROOT_CLASS is named as \c Foo . At the same time, it
 * also requires you to defines a range for drived classes of the class
 * represented by \c DERIVED_CLASS in your kind enum. The lower bound of
 * this range shall be named as \c First_Bar and the upper bound shall be
 * named \c Last_Bar if the class represented by \c DERIVED_CLASS is named
 * \c Bar .
 *
 */
#define LLVM_RTTI_CLASSOF_NONLEAF_CLASS(ROOT_CLASS, DERIVED_CLASS)       \
  static bool classof(const ROOT_CLASS * I) {                            \
    if (I == nullptr) {                                                  \
      return false;                                                      \
    }                                                                    \
    return ROOT_CLASS##Kind::First_##DERIVED_CLASS <= I->getKind()       \
        && I->getKind() <= ROOT_CLASS##Kind::Last_##DERIVED_CLASS;       \
  }

/**
 * @brief Declares the \c classof static function of LLVM-style RTTI for
 * leaf class of kind \c CLASS_KIND whose root class is \c ROOT_CLASS .
 *
 * @note This macros requires you to define the kind enum (result type of
 * \c getKind() function) to be named as \c FooKind if the class
 * represented by \c ROOT_CLASS is named as \c Foo . At the same time, it
 * also requires you to define \c CLASS_KIND as the case name of the kind
 * enum which represents the leaf class.
 *
 */
#define LLVM_RTTI_CLASSOF_LEAF_CLASS(ROOT_CLASS, CLASS_KIND)             \
  static bool classof(const ROOT_CLASS * I) {                            \
    if (I == nullptr) {                                                  \
      return false;                                                      \
    }                                                                    \
    return I->getKind() == ROOT_CLASS##Kind::CLASS_KIND;                 \
  }

#endif // W2N_BASIC_LLVMRTTI_H