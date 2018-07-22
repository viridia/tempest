#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_H
#define TEMPEST_SEMA_GRAPH_TYPESTORE_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
  #include "tempest/sema/graph/type.h"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  class Env;
  class Member;
  class ParameterDefn;

  /** A hashable tuple of types that can be used as a lookup key. */
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
    llvm::ArrayRef<Type*> _members;
  };

  inline void hash_combine(size_t& lhs, size_t rhs) {
    lhs ^= rhs + 0x9e3779b9 + (lhs<<6) + (lhs>>2);
  }

  struct TypeKeyHash {
    inline std::size_t operator()(const TypeKey& value) const {
      std::size_t seed = 0;
      for (tempest::sema::graph::Type* member : value) {
        hash_combine(seed, std::hash<tempest::sema::graph::Type*>()(member));
      }
      return seed;
    }
  };

  /** A hashable tuple of types that can be used as a lookup key. */
  class SpecializationKey {
  public:
    SpecializationKey() = default;
    SpecializationKey(const SpecializationKey& key) = default;
    SpecializationKey(const GenericDefn* base, const TypeArray& typeArgs)
      : _base(base)
      , _typeArgs(TypeKey(typeArgs))
    {}

    /** The generic type to be specialized. */
    const GenericDefn* base() const { return _base; }

    /** The type arguments to the generic type. */
    const TypeKey typeArgs() const { return _typeArgs; }

    /** Equality comparison. */
    friend bool operator==(const SpecializationKey& lhs, const SpecializationKey& rhs) {
      return lhs._base == rhs._base && lhs._typeArgs == rhs._typeArgs;
    }

    /** Inequality comparison. */
    friend bool operator!=(const SpecializationKey& lhs, const SpecializationKey& rhs) {
      return lhs._base != rhs._base || lhs._typeArgs != rhs._typeArgs;
    }

  private:
    const GenericDefn* _base;
    TypeKey _typeArgs;
  };

  struct SpecializationKeyHash {
    inline std::size_t operator()(const SpecializationKey& value) const {
      std::size_t hash = std::hash<const tempest::sema::graph::GenericDefn*>()(value.base());
      hash_combine(hash, TypeKeyHash()(value.typeArgs()));
      return hash;
    }
  };

  /** A store of canonicalized, uniqued derived types. */
  class TypeStore {
  public:
    /** TypeStore has its own allocator. */
    llvm::BumpPtrAllocator& alloc() { return _alloc; }

    /** Create an environment object from a set of type mappings. */
    Env createEnv(const Env& env);

    /** Create a union type from the given type key. */
    UnionType* createUnionType(const TypeArray& members);

    /** Create a tuple type from the given type key. */
    TupleType* createTupleType(const TypeArray& members);

    /** Specialize a generic definition. */
    SpecializedDefn* specialize(GenericDefn* base, const TypeArray& typeArgs);

    /** Create a function type from a return type and parameter types. */
    FunctionType* createFunctionType(Type* returnType, const TypeArray& paramTypes);

    /** Create a function type from a return type and a parameter list. */
    FunctionType* createFunctionType(
        Type* returnType,
        const llvm::ArrayRef<ParameterDefn*>& params);

    /** Create a const type. */
    ConstType* createConstType(Type* base, bool provisional);

  private:
    typedef std::pair<Type*, bool> ConstKey;
    struct ConstKeyHash {
      inline std::size_t operator()(const ConstKey& value) const {
        std::size_t result = std::hash<Type*>()(value.first);
        hash_combine(result, std::hash<bool>()(value.second));
        return result;
      }
    };
    struct ConstKeyEqual {
      inline bool operator()(const ConstKey& key0, const ConstKey& key1) const {
        return key0.first == key1.first && key0.second == key1.second;
      }
    };

    llvm::BumpPtrAllocator _alloc;
    std::unordered_map<TypeKey, UnionType*, TypeKeyHash> _unionTypes;
    std::unordered_map<TypeKey, TupleType*, TypeKeyHash> _tupleTypes;
    std::unordered_map<TypeKey, FunctionType*, TypeKeyHash> _functionTypes;
    std::unordered_map<ConstKey, ConstType*, ConstKeyHash> _constTypes;
    std::unordered_map<SpecializationKey, SpecializedDefn*, SpecializationKeyHash> _specs;
    std::unordered_set<Type*> _addressTypes;
  //     self.valueRefTypes = {}
  };
}

#endif
