#ifndef TEMPEST_INTRINSIC_INTRINSIC_HPP
#define TEMPEST_INTRINSIC_INTRINSIC_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

namespace tempest::intrinsic {
  /** Indicates that the type definition is a builtin type .*/
  enum class IntrinsicType : int8_t {
    NONE = 0,

    // Built-in types
    OBJECT_CLASS,
    FLEXALLOC_CLASS,

    // Build-in traits
    ADDITION_TRAIT,
    SUBTRACTION_TRAIT,
    MULTIPLICATION_TRAIT,
    DIVISION_TRAIT,
    MODULUS_TRAIT,

    // Externally-declared intrinsic types
    ADDRESS_TYPE,
    ITERABLE_TYPE,
    ITERATOR_TYPE,
  };

  /** Intrinsic function IDs. */
  enum class IntrinsicFn : int8_t {
    NONE = 0,

    // Operators
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    REMAINDER,
    LSHIFT,
    RSHIFT,
    BIT_OR,
    BIT_AND,
    BIT_XOR,
    EQ,
    LE,
    LT,
    NEGATE,
    COMPLEMENT,

    // SIN
    // COS
    // TYPEOF

    // ADDRESS_OF
    // MEMFILL
    // MEMCPY
    // MEMMOVE
    // FLEX_ALLOC

    // Builtin methods
    OBJECT_CTOR,

    FLEX_ALLOC,
    FLEX_DATA,
    FLEX_ELEMENT,

    MEMORY_COPY,
    MEMORY_MOVE,
    MEMORY_FILL,
  };

  struct ExternalIntrinsicFn {
    StringRef name; // Fully-qualified name
    IntrinsicFn intrinsic;
  };

  struct ExternalIntrinsicType {
    StringRef name; // Fully-qualified name
    IntrinsicType intrinsic;
  };
}

#endif
