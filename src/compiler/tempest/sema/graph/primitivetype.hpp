#ifndef TEMPEST_SEMA_GRAPH_PRIMITIVETYPE_HPP
#define TEMPEST_SEMA_GRAPH_PRIMITIVETYPE_HPP 1

#ifndef TEMPEST_CONFIG_HPP
  #include "config.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

namespace tempest::sema::graph {

  /** Base class for primitive types. */
  class PrimitiveType : public Type {
  public:
    PrimitiveType(Kind kind, const llvm::StringRef& name)
      : Type(kind)
      , _defn(source::Location(), name, nullptr)
    {
      _defn.setType(this);
    }

    /** Name of this primitive type. */
    llvm::StringRef name() const { return _defn.name(); }

    /** Type definition for this primitive type. */
    TypeDefn* defn() { return &_defn; }
    const TypeDefn* defn() const { return &_defn; }

    /** Members of this primitive type (like maxval). */
    SymbolTable* memberScope() { return _defn.memberScope(); }

    /** Lazily-constructed scope for all primitive types. */
    static SymbolTable* scope();

    /** Dynamic casting support. */
    static bool classof(const PrimitiveType* t) { return true; }
    static bool classof(const Type* t) {
      return t->kind >= Kind::VOID && t->kind <= Kind::FLOAT;
    }

  private:
    TypeDefn _defn;
  };

  /** The 'void' type. */
  class VoidType : public PrimitiveType {
  public:
    VoidType()
      : PrimitiveType(Kind::VOID, "void")
    {}

    /** Dynamic casting support. */
    static bool classof(const VoidType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::VOID; }

    static VoidType VOID;
  };

  /** A boolean type. */
  class BooleanType : public PrimitiveType {
  public:
    BooleanType() : PrimitiveType(Kind::BOOLEAN, "bool") {}

    /** Dynamic casting support. */
    static bool classof(const BooleanType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::BOOLEAN; }

    static BooleanType BOOL;
  };

  /** An integer type. There are 'fixed' integer types which represent specific machine word
      sizes, as well as 'implicitly-sized' integer types which are used for constant integer
      expressions (1 + 1) where an integer size has not yet been chosen.

      Implicitly-sized integers are treated as being of unlimited maximum size; however they have
      a *minimum* size based on the integer value they are currently holding. The `bits' field
      contains the number of bits needed to represent the integer constant as a signed number,
      and can be any number greater than zero. The 'isNegative' property indicates whether the
      integer constants is negative; if it is, then it is illegal to assign that type to an
      unsigned fixed integer type.

      The meaning of 'isUnsigned' is different for fixed-sized and implicitly-sized integer types.
      For fixed types, signed and unsigned types cannot be mixed. For implicit types, signed and
      unsigned can be mixed freely, however if any term is unsigned, then the final result is
      unsigned. In other words, for implicit types, 'unsigned' is just a hint as to what the final
      result type should be, and is not a constraint on the values themselves.

      Thus, the following expression is valid:

        const a = 255u - 1;

      The value of `a` will be 254 (255 - 1), and the type will be unsigned. Since the final result
      is not negative, the conversion to unsigned succeeds.

      Note that all integers must be converted to a fixed type before being assigned to a variable,
      passed as an argument to a function, bound to a template parameter, or used as a non-constant.
      Implicit types only exist within arithmetic expressions that are entirely composed of
      constant integers with no specified integer type.
  */
  class IntegerType : public PrimitiveType {
  public:
    IntegerType(llvm::StringRef name, int32_t bits, bool isUnsigned)
      : PrimitiveType(Kind::INTEGER, name)
      , _bits(bits)
      , _unsigned(isUnsigned)
      , _negative(false)
      , _implicitlySized(false)
    {}
    IntegerType(llvm::StringRef name, int32_t bits, bool isUnsigned, bool isNegative)
      : PrimitiveType(Kind::INTEGER, name)
      , _bits(bits)
      , _unsigned(isUnsigned)
      , _negative(isNegative)
      , _implicitlySized(true)
    {}

    /** Number of bits in this integer type. */
    int32_t bits() const { return _bits; }

    /** If true, this is an unsigned integer type. */
    bool isUnsigned() const { return _unsigned; }

    /** If true, this is the type of an integer constant of indeterminate size. */
    bool isImplicitlySized() const { return _implicitlySized; }

    /** If true, the integer constant is a negative number. */
    int32_t isNegative() const { return _negative; }

    /** Dynamic casting support. */
    static bool classof(const IntegerType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::INTEGER; }

    static IntegerType CHAR;
    static IntegerType I8;
    static IntegerType I16;
    static IntegerType I32;
    static IntegerType I64;
    static IntegerType U8;
    static IntegerType U16;
    static IntegerType U32;
    static IntegerType U64;

  private:
    int32_t _bits;
    bool _unsigned;
    bool _negative;
    bool _implicitlySized;
  };

  /** A floating-point type. */
  class FloatType : public PrimitiveType {
  public:
    FloatType(llvm::StringRef name, int32_t bits)
      : PrimitiveType(Kind::FLOAT, name)
      , _bits(bits)
    {}

    /** Number of bits in this floating-point type. */
    int32_t bits() const { return _bits; }

    /** Dynamic casting support. */
    static bool classof(const FloatType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::FLOAT; }

    static FloatType F32;
    static FloatType F64;

  private:
    int32_t _bits;
  };

// void createConstants(support::Arena& arena);

}

#endif
