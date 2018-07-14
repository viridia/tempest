// ============================================================================
// sema/graph/type.h: Semantic graph nodes for types.
// ============================================================================

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
#define TEMPEST_SEMA_GRAPH_TYPE_H 1

#ifndef TEMPEST_CONFIG_H
  #include "config.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_ENV_H
  #include "tempest/sema/graph/env.h"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

namespace tempest::sema::graph {

  class TypeDefn;
  class TypeParameter;

  /** Base class for all types. */
  class Type {
  public:
    enum class Kind {
      INVALID = 0,
      NEVER,          // Sentinel type used to indicate expression never returns
      IGNORED,        // Sentinel type used to indicate value is ignored.

      // Nominal types
      VOID,
      BOOLEAN, INTEGER, FLOAT,    // Primitives
      CLASS, STRUCT, INTERFACE, TRAIT, EXTENSION, ENUM,  // Composites
      ALIAS,          // An alias for another type
      TYPE_VAR,       // Reference to a template parameter

      // Derived types
      UNION,          // Disjoint types
      INTERSECTION,   // Intersection types
      TUPLE,          // Tuple of types
      FUNCTION,       // Function type
      CONST,          // Const type modifier
      SPECIALIZED,    // Instantiation of a type
      VALUE_PARAM,    // A type parameter bound to an immutable value

      // Types used internally during compilation
  //     TYPE_EXPR,  // A type represented as an expression
      CONTINGENT,     // May be one of several types depending on overload selection.
  //     AMBIGUOUS,  // Ambiguous type
  //     PHI,        // Represents multiple types produced by different code paths.

      TYPESET,        // Represents a set of overloaded types with the same name.

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
    static Type NO_RETURN;
  };

  /** Array of types. */
  typedef llvm::ArrayRef<Type*> TypeArray;

  /** User-defined types such as classes, structs, interfaces and enumerations. */
  class UserDefinedType : public Type {
  public:
    UserDefinedType(Kind kind)
      : Type(kind)
      , _defn(nullptr)
    {}

    /** Definition for this type. */
    TypeDefn* defn() const { return _defn; }
    void setDefn(TypeDefn* defn) { _defn = defn; }

    /** Return the array of types that this type extends. */
    TypeArray extends() const { return _extends; }
    void setExtends(TypeArray types) {
      _extends.assign(types.begin(), types.end());
    }

    /** Return the array of types that this type implements. */
    TypeArray implements() const { return _implements; }
    void setImplements(TypeArray types) {
      _implements.assign(types.begin(), types.end());
    }

    /** Dynamic casting support. */
    static bool classof(const UserDefinedType* t) { return true; }
    static bool classof(const Type* t) { return t->kind >= Kind::CLASS && t->kind <= Kind::ENUM; }

  private:
    TypeDefn* _defn;
    llvm::SmallVector<Type*, 4> _extends;
    llvm::SmallVector<Type*, 4> _implements;
  };

  /** Union type. */
  class UnionType : public Type {
  public:
    /** Array of members of this union or tuple. */
    const llvm::SmallVector<Type*, 4> members;

    UnionType(const TypeArray& members)
      : Type(Kind::UNION)
      , members(members.begin(), members.end()) {}

    /** Dynamic casting support. */
    static bool classof(const UnionType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::UNION; }
  };

  /** Intersection type. */
  class IntersectionType : public Type {
  public:
    /** Array of members of this union or tuple. */
    const llvm::SmallVector<Type*, 4> members;

    IntersectionType(const TypeArray& members)
      : Type(Kind::INTERSECTION)
      , members(members.begin(), members.end()) {}

    /** Dynamic casting support. */
    static bool classof(const IntersectionType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::INTERSECTION; }
  };

  /** Tuple type. */
  class TupleType : public Type {
  public:
    /** Array of members of this union or tuple. */
    const llvm::SmallVector<Type*, 4> members;

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
    const llvm::SmallVector<Type*, 4> paramTypes;

    /** True if this function cannot modify its context. */
    const bool constSelf;

    FunctionType(Type* returnType, const TypeArray& paramTypes, bool isConstSelf)
      : Type(Kind::FUNCTION)
      , returnType(returnType)
      , paramTypes(paramTypes.begin(), paramTypes.end())
      , constSelf(isConstSelf)
    {}

    /** Dynamic casting support. */
    static bool classof(const FunctionType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::FUNCTION; }
  };

  /** A type that represents a unique constant value. */
  // class ValueParamType : public Type {
  //   value : expr.Expr = 1;
  // }

  /** Constant modifier on a type. */
  class ConstType : public Type {
  public:
    ConstType(Type* base, bool provisional = false)
      : Type(Kind::CONST)
      , _base(base)
      , _provisional(provisional)
    {}

    /** The type being modified. */
    Type* base() const { return _base; }

    /** Only const if outer scope is too. */
    bool provisional() const { return _provisional; }

  private:
    Type* _base;
    bool _provisional;
  };

  /** Type variables are used in type expressions to represent a template parameter. */
  class TypeVar : public Type {
  public:
    /** The type parameter associated with this type variable. */
    const TypeParameter* param;

    TypeVar(TypeParameter* param) : Type(Kind::TYPE_VAR), param(param) {}

    /** Dynamic casting support. */
    static bool classof(const TypeVar* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::TYPE_VAR; }
  };

  /** A tuple of types that can be used as a lookup key. */
  class TypeKey {
  public:
    TypeKey() {}
    TypeKey(const TypeArray& members) : _members(members.begin(), members.end()) {}
    TypeKey(const TypeKey& key) : _members(key._members) {}

    /** Assignment operator. */
    TypeKey& operator=(const TypeKey& key) {
      _members = key._members;
      return *this;
    }

    /** Equality comparison. */
    friend bool operator==(const TypeKey& lhs, const TypeKey& rhs) {
      return lhs._members == rhs._members;
    }

    /** Inequality comparison. */
    friend bool operator!=(const TypeKey& lhs, const TypeKey& rhs) {
      return lhs._members != rhs._members;
    }

    /** Iteration. */
    TypeArray::const_iterator begin() const { return _members.begin(); }
    TypeArray::const_iterator end() const { return _members.end(); }

    /** Return the length of the key. */
    size_t size() const { return _members.size(); }

    /** Read-only random access. */
    Type* operator[](int index) const {
      return _members[index];
    }

  private:
    llvm::SmallVector<Type*, 4> _members;
  };
}

#endif
