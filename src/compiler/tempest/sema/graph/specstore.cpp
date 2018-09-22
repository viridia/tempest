#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/specstore.hpp"
#include "tempest/sema/graph/typeorder.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::graph {
  using tempest::error::diag;

  SpecializationStore::~SpecializationStore() {
    _specs.clear();
  }

  SpecializedDefn* SpecializationStore::specialize(GenericDefn* base, const TypeArray& typeArgs) {
    SpecializationKey key(base, typeArgs);
    auto it = _specs.find(key);
    if (it != _specs.end()) {
      return it->second;
    }

    auto spec = new (_alloc) SpecializedDefn(base, _alloc.copyOf(typeArgs), base->allTypeParams());
    if (auto typeDefn = llvm::dyn_cast<TypeDefn>(base)) {
      spec->setType(new (_alloc) SpecializedType(spec));
    }
    SpecializationKey newKey(base, _alloc.copyOf(typeArgs));
    _specs[newKey] = spec;
    return spec;
  }

  void SpecializationStore::addConcreteSpec(SpecializedDefn* sp) {
    auto result = _concreteSpecs.insert(sp);
    if (result.second) {
      _concreteSpecList.push_back(sp);
    }
  }
}
