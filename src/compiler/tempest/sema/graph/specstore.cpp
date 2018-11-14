#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/specstore.hpp"
#include "tempest/sema/graph/typeorder.hpp"
#include "llvm/Support/Casting.h"
#include <cassert>

namespace tempest::sema::graph {
  using tempest::error::diag;

  SpecializationStore::~SpecializationStore() {
    _specs.clear();
  }

  SpecializedDefn* SpecializationStore::specialize(GenericDefn* base, const TypeArray& typeArgs) {
    assert(!typeArgs.empty());
    assert(isa<GenericDefn>(base));
    SpecializationKey<Defn> key(base, typeArgs);
    auto it = _specs.find(key);
    if (it != _specs.end()) {
      return it->second;
    }

    auto spec = new (_alloc) SpecializedDefn(base, _alloc.copyOf(typeArgs), base->allTypeParams());
    if (auto typeDefn = llvm::dyn_cast<TypeDefn>(base)) {
      spec->setType(new (_alloc) SpecializedType(spec));
    }
    SpecializationKey<Defn> newKey(base, _alloc.copyOf(typeArgs));
    _specs[newKey] = spec;
    return spec;
  }

  SpecializedDefn* SpecializationStore::specialize(Defn* base, const TypeArray& typeArgs) {
    assert(!typeArgs.empty());
    GenericDefn* genericParent = nullptr;
    for (Member* m = base; m; m = m->definedIn()) {
      if (auto generic = dyn_cast<GenericDefn>(m)) {
        genericParent = generic;
      }
    }

    assert(genericParent);
    SpecializationKey<Defn> key(base, typeArgs);
    auto it = _specs.find(key);
    if (it != _specs.end()) {
      return it->second;
    }

    auto spec = new (_alloc)
        SpecializedDefn(base, _alloc.copyOf(typeArgs), genericParent->allTypeParams());
    if (auto typeDefn = llvm::dyn_cast<TypeDefn>(base)) {
      spec->setType(new (_alloc) SpecializedType(spec));
    }
    SpecializationKey<Defn> newKey(base, _alloc.copyOf(typeArgs));
    _specs[newKey] = spec;
    return spec;
  }
}
