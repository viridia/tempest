#ifndef TEMPEST_INTRINSIC_INTRINSIC_HPP
#define TEMPEST_INTRINSIC_INTRINSIC_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

namespace tempest::intrinsic {
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
  };
}

#endif
