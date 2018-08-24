#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
#define TEMPEST_SEMA_GRAPH_TYPE_HPP 1

#ifndef TEMPEST_CONFIG_HPP
  #include "config.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_ENV_HPP
  #include "tempest/sema/graph/env.hpp"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

namespace tempest::sema::graph {

  class Expr;
  class SpecializedDefn;
  class TypeDefn;
  class TypeParameter;

  /** Base class for all types. */
  class Type {
  public:
    enum class Kind {
      INVALID = 0,
      NEVER,          // Sentinel type used to indicate expression never returns
      IGNORED,        // Sentinel type used to indicate value is ignored.
      NOT_EXPR,       // Sentinel type used to indicate value is not an expression.

      // Nominal types
      VOID,
      BOOLEAN, INTEGER, FLOAT,    // Primitives
      CLASS, STRUCT, INTERFACE, TRAIT, EXTENSION, ENUM,  // Composites
      ALIAS,          // An alias for another type
      TYPE_VAR,       // Reference to a template parameter

      // Derived types
      UNION,          // Disjoint types
      TUPLE,          // Tuple of types
      FUNCTION,       // Function type
      MODIFIED,       // Readonly or immutable type modifier
      SPECIALIZED,    // Specialization of a generic user-defined type
      // WITH_ARGS,      // A type specialization, i.e X[A]
      SINGLETON,      // A type consisting of a single value

      // Types used during type inference
      CONTINGENT,     // May be one of several types depending on overload selection.
      RENAMED,        // A renamed type var during inference.
  //     PHI,        // Represents multiple types produced by different code paths.
  //     AMBIGUOUS,  // Ambiguous type

      // TYPESET,        // Represents a set of overloaded types with the same name.

      // Backend - used during code generation
  //     VALUE_REF,
  //     ADDRESS,
    };

    Type(Kind kind) : kind(kind) {}
    Type() = delete;
    Type(const Type& src) = delete;

    const Kind kind;

    /** Return true if this type is an error sentinel. */
    static bool isError(const Type* t) {
      return t == nullptr || t->kind == Kind::INVALID;
    }

    /** Dynamic casting support. */
    static bool classof(const Type* t) { return true; }

    static Type ERROR;
    static Type IGNORED;
    static Type NOT_EXPR;
    static Type NO_RETURN;
  };

  /** Function to print a type. */
  void format(::std::ostream& out, const Type* t);

  /** Array of types. */
  typedef llvm::ArrayRef<const Type*> TypeArray;

  /** User-defined types such as classes, structs, interfaces and enumerations. */
  class UserDefinedType : public Type {
  public:
    UserDefinedType(Kind kind)
      : Type(kind)
      , _defn(nullptr)
    {}

    UserDefinedType(Kind kind, TypeDefn* defn)
      : Type(kind)
      , _defn(defn)
    {}

    /** Definition for this type. */
    TypeDefn* defn() const { return _defn; }
    void setDefn(TypeDefn* defn) { _defn = defn; }

    /** Dynamic casting support. */
    static bool classof(const UserDefinedType* t) { return true; }
    static bool classof(const Type* t) { return t->kind >= Kind::CLASS && t->kind <= Kind::ALIAS; }

  private:
    TypeDefn* _defn;
  };

  /** Union type. */
  class UnionType : public Type {
  public:
    /** Array of members of this union or tuple. */
    const llvm::SmallVector<const Type*, 4> members;

    UnionType(const TypeArray& members)
      : Type(Kind::UNION)
      , members(members.begin(), members.end()) {}

    /** Dynamic casting support. */
    static bool classof(const UnionType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::UNION; }
  };

  /** Tuple type. */
  class TupleType : public Type {
  public:
    /** Array of members of this union or tuple. */
    const llvm::SmallVector<const Type*, 4> members;

    TupleType(const TypeArray& members)
      : Type(Kind::TUPLE)
      , members(members.begin(), members.end()) {}

    /** Dynamic casting support. */
    static bool classof(const TupleType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::TUPLE; }
  };

  /** Type of a function. */
  class FunctionType : public Type {
  public:
    const Type* returnType;

    /** The type of this function's parameters. */
    const llvm::SmallVector<const Type*, 4> paramTypes;

    /** True if this function cannot modify its context. */
    const bool constSelf;

    /** True if the last parameter of this function is variadic. */
    const bool isVariadic;

    FunctionType(
        const Type* returnType, const TypeArray& paramTypes, bool isConstSelf, bool isVariadic)
      : Type(Kind::FUNCTION)
      , returnType(returnType)
      , paramTypes(paramTypes.begin(), paramTypes.end())
      , constSelf(isConstSelf)
      , isVariadic(isVariadic)
    {}

    /** Dynamic casting support. */
    static bool classof(const FunctionType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::FUNCTION; }
  };

  /** A type that represents a unique constant value. */
  // class ValueParamType : public Type {
  //   value : expr.Expr = 1;
  // }

  /** Readonly or immutable modifier on a type. */
  class ModifiedType : public Type {
  public:
    enum Modifiers {
      READ_ONLY = 1 << 0,
      IMMUTABLE = 1 << 1,
    };

    const Type* base;
    const uint32_t modifiers;

    bool isReadOnly() const { return (modifiers & READ_ONLY) != 0; }
    bool isImmutable() const { return (modifiers & IMMUTABLE) != 0; }

    ModifiedType(const Type* base, uint32_t modifiers)
      : Type(Kind::MODIFIED)
      , base(base)
      , modifiers(modifiers)
    {}
  };

  /** A type that represents a single value. */
  class SingletonType : public Type {
  public:
    /** Value of this singleton; must be a constant. */
    const Expr* value;

    SingletonType(const Expr* value)
      : Type(Kind::SINGLETON)
      , value(value) {}

    /** Dynamic casting support. */
    static bool classof(const SingletonType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::SINGLETON; }
  };

  /** Type variables are used in type expressions to represent a template parameter. */
  class TypeVar : public Type {
  public:
    /** The type parameter associated with this type variable. */
    const TypeParameter* param;

    TypeVar(TypeParameter* param) : Type(Kind::TYPE_VAR), param(param) {}

    /** The ordinal index of this type variable relative to other type variables. */
    int32_t index() const;

    /** Dynamic casting support. */
    static bool classof(const TypeVar* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::TYPE_VAR; }
  };

  /** A reference to a specialized user-defined type, generated from TypeWithArgs. */
  class SpecializedType : public Type {
  public:
    /** Reference to the specialized definition. */
    const SpecializedDefn* spec;

    SpecializedType(SpecializedDefn* spec)
      : Type(Kind::SPECIALIZED)
      , spec(spec)
    {}

    /** Dynamic casting support. */
    static bool classof(const SpecializedType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::SPECIALIZED; }
  };

  /** A type + type arguments. Only used when we know nothing about the base type. */
  // class TypeWithArgs : public Type {
  // public:
  //   /** Type being specializd; most likely a UserDefinedType. */
  //   const Type* base;
  //   const llvm::SmallVector<Type*, 4> typeArgs;

  //   TypeWithArgs(Type* base, llvm::ArrayRef<Type*>& typeArgs)
  //     : Type(Kind::WITH_ARGS)
  //     , base(base)
  //     , typeArgs(typeArgs.begin(), typeArgs.end())
  //   {}

  //   /** Dynamic casting support. */
  //   static bool classof(const TypeWithArgs* t) { return true; }
  //   static bool classof(const Type* t) { return t->kind == Kind::WITH_ARGS; }
  // };
  inline ::std::ostream& operator<<(::std::ostream& os, const Type* t) {
    format(os, t);
    return os;
  }
}

#endif
