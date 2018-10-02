#ifndef TEMPEST_SEMA_GRAPH_SPECKEY_HPP
#define TEMPEST_SEMA_GRAPH_SPECKEY_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPEKEY_HPP
  #include "tempest/sema/graph/typekey.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  using tempest::support::hash_combine;

  struct Env;

  /** Key used to lookup a specialized object via its type arguments. */
  template<class T>
  class SpecializationKey {
  public:
    SpecializationKey() = default;
    SpecializationKey(const SpecializationKey& key) = default;
    SpecializationKey(const T* base, const TypeArray& typeArgs)
      : _base(base)
      , _typeArgs(TypeKey(typeArgs))
    {}

    /** The generic type to be specialized. */
    const T* base() const { return _base; }

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
    const T* _base;
    TypeKey _typeArgs;
  };

  template<class T>
  struct SpecializationKeyHash {
    inline std::size_t operator()(const SpecializationKey<T>& value) const {
      std::size_t hash = std::hash<const T*>()(value.base());
      hash_combine(hash, TypeKeyHash()(value.typeArgs()));
      return hash;
    }
  };
}

#endif
