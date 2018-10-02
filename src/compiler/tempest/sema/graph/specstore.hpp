#ifndef TEMPEST_SEMA_GRAPH_SPECSTORE_HPP
#define TEMPEST_SEMA_GRAPH_SPECSTORE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SPECKEY_HPP
  #include "tempest/sema/graph/speckey.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  using tempest::support::hash_combine;

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
    std::unordered_map<SpecializationKey<Defn>, SpecializedDefn*, SpecializationKeyHash<Defn>>&
        specializations() { return _specs; }

  private:
    tempest::support::BumpPtrAllocator& _alloc;
    std::unordered_map<SpecializationKey<Defn>, SpecializedDefn*, SpecializationKeyHash<Defn>>
        _specs;
  };
}

#endif
