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

#ifndef TEMPEST_SEMA_GRAPH_TYPEKEY_HPP
  #include "tempest/sema/graph/typekey.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

#ifndef TEMPEST_SUPPORT_HASHING_HPP
  #include "tempest/support/hashing.hpp"
#endif

#include <unordered_map>
#include <unordered_set>

namespace tempest::sema::graph {
  using tempest::support::hash_combine;

  struct Env;
  class IntegerType;

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

  struct IntKey {
    int32_t bits;
    bool isNegative;
    bool isUnsigned;
  };

  /** A store of canonicalized, uniqued derived types. */
  class TypeStore {
  public:
    ~TypeStore();

    /** TypeStore has its own allocator. */
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    /** Create a union type from the given type key. */
    UnionType* createUnionType(const TypeArray& members);

    /** Create a tuple type from the given type key. */
    TupleType* createTupleType(const TypeArray& members);

    /** Create a singleton type. */
    SingletonType* createSingletonType(const Expr* expr);

    /** Create an integer type large enough to hold an integer value. */
    IntegerType* createIntegerType(llvm::APInt& intVal, bool isUnsigned);

    /** Create a function type from a return type and parameter types. */
    FunctionType* createFunctionType(
        const Type* returnType,
        const TypeArray& paramTypes,
        bool isMutableSelf = false,
        bool isVariadic = false);

    /** Create a const type. */
    const ModifiedType* createModifiedType(const Type* base, uint32_t modifiers);

  private:
    typedef std::pair<const Type*, uint32_t> ModifiedKey;
    struct ModifiedKeyHash {
      inline std::size_t operator()(const ModifiedKey& value) const {
        std::size_t result = std::hash<const Type*>()(value.first);
        hash_combine(result, std::hash<uint32_t>()(value.second));
        return result;
      }
    };
    // struct ModifiedKeyEqual {
    //   inline bool operator()(const ModifiedKey& key0, const ModifiedKey& key1) const {
    //     return key0.first == key1.first && key0.second == key1.second;
    //   }
    // };

    struct IntKeyHash {
      inline std::size_t operator()(const IntKey& value) const {
        std::size_t result = std::hash<int32_t>()(value.bits);
        hash_combine(result, std::hash<bool>()(value.isNegative));
        hash_combine(result, std::hash<bool>()(value.isUnsigned));
        return result;
      }
    };

    struct IntKeyEqual {
      inline bool operator()(const IntKey& l, const IntKey& r) const {
        return l.bits == r.bits
            && l.isNegative == r.isNegative
            && l.isUnsigned == r.isUnsigned;
      }
    };

    tempest::support::BumpPtrAllocator _alloc;
    std::unordered_map<IntKey, IntegerType*, IntKeyHash, IntKeyEqual> _intTypes;
    std::unordered_map<TypeKey, UnionType*, TypeKeyHash> _unionTypes;
    std::unordered_map<TypeKey, TupleType*, TypeKeyHash> _tupleTypes;
    std::unordered_map<FunctionTypeKey, FunctionType*, FunctionTypeKeyHash> _functionTypes;
    std::unordered_map<ModifiedKey, ModifiedType*, ModifiedKeyHash> _modifiedTypes;
    std::unordered_map<SingletonKey, SingletonType*, SingletonKeyHash> _singletonTypes;
    std::unordered_map<SpecializedDefn*, SpecializedType*> _specializdTypes;
    std::unordered_set<Type*> _addressTypes;
  //     self.valueRefTypes = {}
  };
}

#endif
