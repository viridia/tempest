#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
#define TEMPEST_SEMA_GRAPH_TYPESTORE_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

#include <unordered_map>
#include <unordered_set>

namespace tempest::sema::graph {
  using tempest::sema::graph::Type;

  struct Env;
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
    const Type* operator[](int index) const {
      return _members[index];
    }

  private:
    llvm::ArrayRef<const Type*> _members;
  };

  inline void hash_combine(size_t& lhs, size_t rhs) {
    lhs ^= rhs + 0x9e3779b9 + (lhs<<6) + (lhs>>2);
  }

  struct TypeKeyHash {
    inline std::size_t operator()(const TypeKey& value) const {
      std::size_t seed = 0;
      for (auto member : value) {
        hash_combine(seed, std::hash<const Type*>()(member));
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

  /** A hashable expression for singletons. */
  class SingletonKey {
  public:
    SingletonKey() = default;
    SingletonKey(const SingletonKey& key) = default;
    SingletonKey(const Expr* value)
      : _value(value)
    {}

    /** The generic type to be specialized. */
    const Expr* value() const { return _value; }

    bool equals(const SingletonKey& sk) const;

    /** Equality comparison. */
    friend bool operator==(const SingletonKey& lhs, const SingletonKey& rhs) {
      return lhs.equals(rhs);
    }

    /** Inequality comparison. */
    friend bool operator!=(const SingletonKey& lhs, const SingletonKey& rhs) {
      return !lhs.equals(rhs);
    }

  private:
    const Expr* _value;
  };

  struct SingletonKeyHash {
    std::size_t operator()(const SingletonKey& value) const;
  };

  /** A store of canonicalized, uniqued derived types. */
  class TypeStore {
  public:
    ~TypeStore();

    /** TypeStore has its own allocator. */
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    /** Create an environment object from a set of type mappings. */
    Env createEnv(const Env& env);

    /** Create a union type from the given type key. */
    UnionType* createUnionType(const TypeArray& members);

    /** Create a tuple type from the given type key. */
    TupleType* createTupleType(const TypeArray& members);

    /** Create a singleton type. */
    SingletonType* createSingletonType(const Expr* expr);

    /** Specialize a generic definition. */
    SpecializedDefn* specialize(GenericDefn* base, const TypeArray& typeArgs);

    /** Create a function type from a return type and parameter types. */
    FunctionType* createFunctionType(
        const Type* returnType,
        const TypeArray& paramTypes,
        bool isVariadic = false);

    /** Create a function type from a return type and a parameter list. */
    // FunctionType* createFunctionType(
    //     Type* returnType,
    //     const llvm::ArrayRef<ParameterDefn*>& params);

    /** Create a const type. */
    ModifiedType* createModifiedType(Type* base, uint32_t modifiers);

  private:
    typedef std::pair<Type*, uint32_t> ModifiedKey;
    struct ModifiedKeyHash {
      inline std::size_t operator()(const ModifiedKey& value) const {
        std::size_t result = std::hash<Type*>()(value.first);
        hash_combine(result, std::hash<uint32_t>()(value.second));
        return result;
      }
    };
    struct ModifiedKeyEqual {
      inline bool operator()(const ModifiedKey& key0, const ModifiedKey& key1) const {
        return key0.first == key1.first && key0.second == key1.second;
      }
    };


    tempest::support::BumpPtrAllocator _alloc;
    std::unordered_map<TypeKey, UnionType*, TypeKeyHash> _unionTypes;
    std::unordered_map<TypeKey, TupleType*, TypeKeyHash> _tupleTypes;
    std::unordered_map<TypeKey, FunctionType*, TypeKeyHash> _functionTypes;
    std::unordered_map<ModifiedKey, ModifiedType*, ModifiedKeyHash> _modifiedTypes;
    std::unordered_map<SingletonKey, SingletonType*, SingletonKeyHash> _singletonTypes;
    std::unordered_map<SpecializationKey, SpecializedDefn*, SpecializationKeyHash> _specs;
    std::unordered_set<Type*> _addressTypes;
  //     self.valueRefTypes = {}
  };
}

#endif
