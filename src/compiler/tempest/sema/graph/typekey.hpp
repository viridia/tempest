#ifndef TEMPEST_SEMA_GRAPH_TYPEKEY_HPP
#define TEMPEST_SEMA_GRAPH_TYPEKEY_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SUPPORT_HASHING_HPP
  #include "tempest/support/hashing.hpp"
#endif

namespace tempest::sema::graph {
  using tempest::support::hash_combine;

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

    TypeKey& operator=(TypeKey&& key) {
      _members = std::move(key._members);
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

  struct TypeKeyHash {
    inline std::size_t operator()(const TypeKey& value) const {
      std::size_t seed = 0;
      for (auto member : value) {
        hash_combine(seed, std::hash<const Type*>()(member));
      }
      return seed;
    }
  };

  struct FunctionTypeKey {
    TypeKey params;
    bool isMutableSelf;
    bool isVariadic;

    FunctionTypeKey() {}
    FunctionTypeKey(const TypeArray& members, bool isMutableSelf, bool isVariadic)
      : params(members), isMutableSelf(isMutableSelf), isVariadic(isVariadic) {}
    FunctionTypeKey(const FunctionTypeKey& key)
      : params(key.params), isMutableSelf(key.isMutableSelf), isVariadic(key.isVariadic) {}

    /** Assignment operator. */
    FunctionTypeKey& operator=(const FunctionTypeKey& key) {
      params = key.params;
      isMutableSelf = key.isMutableSelf;
      isVariadic = key.isVariadic;
      return *this;
    }

    FunctionTypeKey& operator=(FunctionTypeKey&& key) {
      params = std::move(key.params);
      isMutableSelf = key.isMutableSelf;
      isVariadic = key.isVariadic;
      return *this;
    }

    /** Equality comparison. */
    friend bool operator==(const FunctionTypeKey& lhs, const FunctionTypeKey& rhs) {
      return lhs.params == rhs.params
          && lhs.isMutableSelf == rhs.isMutableSelf
          && lhs.isVariadic == rhs.isVariadic;
    }

    /** Inequality comparison. */
    friend bool operator!=(const FunctionTypeKey& lhs, const FunctionTypeKey& rhs) {
      return lhs.params != rhs.params
          || lhs.isMutableSelf != rhs.isMutableSelf
          || lhs.isVariadic != rhs.isVariadic;
    }
  };

  struct FunctionTypeKeyHash {
    inline std::size_t operator()(const FunctionTypeKey& value) const {
      std::size_t seed = 0;
      for (auto member : value.params) {
        hash_combine(seed, std::hash<const Type*>()(member));
      }
      hash_combine(seed, std::hash<bool>()(value.isMutableSelf));
      hash_combine(seed, std::hash<bool>()(value.isVariadic));
      return seed;
    }
  };

}

#endif
