#ifndef TEMPEST_SEMA_GRAPH_SPECSTORE_HPP
#define TEMPEST_SEMA_GRAPH_SPECSTORE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  using tempest::support::hash_combine;

  struct Env;

  /** A hashable tuple of types that can be used as a lookup key. */
  class SpecializationKey {
  public:
    SpecializationKey() = default;
    SpecializationKey(const SpecializationKey& key) = default;
    SpecializationKey(const Defn* base, const TypeArray& typeArgs)
      : _base(base)
      , _typeArgs(TypeKey(typeArgs))
    {}

    /** The generic type to be specialized. */
    const Defn* base() const { return _base; }

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
    const Defn* _base;
    TypeKey _typeArgs;
  };

  struct SpecializationKeyHash {
    inline std::size_t operator()(const SpecializationKey& value) const {
      std::size_t hash = std::hash<const tempest::sema::graph::Defn*>()(value.base());
      hash_combine(hash, TypeKeyHash()(value.typeArgs()));
      return hash;
    }
  };

  struct SpecializationPtrHash {
    inline std::size_t operator()(const SpecializedDefn* value) const {
      std::size_t hash = std::hash<const tempest::sema::graph::Member*>()(value->generic());
      hash_combine(hash, TypeKeyHash()(value->typeArgs()));
      return hash;
    }
  };

  /** A store of canonicalized, uniqued derived types. */
  class SpecializationStore {
  public:
    SpecializationStore(tempest::support::BumpPtrAllocator& alloc) : _alloc(alloc) {}
    ~SpecializationStore();

    /** TypeStore has its own allocator. */
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    /** Specialize a generic definition. */
    SpecializedDefn* specialize(GenericDefn* base, const TypeArray& typeArgs);

    /** The map of all specializations. */
    std::unordered_map<SpecializationKey, SpecializedDefn*, SpecializationKeyHash>&
        specializations() { return _specs; }

  private:
    tempest::support::BumpPtrAllocator& _alloc;
    std::unordered_map<SpecializationKey, SpecializedDefn*, SpecializationKeyHash> _specs;
  };
}

#endif
