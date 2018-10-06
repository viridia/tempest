#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/env.hpp"
#include "tempest/sema/graph/type.hpp"

namespace tempest::sema::graph {

  bool Env::has(const TypeVar* tvar) {
    return has(tvar->param);
  }

  bool Env::has(const TypeParameter* param) {
    return std::find(params.begin(), params.end(), param) != params.end();
  }

  const Type* Env::get(const TypeVar* tvar) {
    return get(tvar->param);
  }

  const Type* Env::get(const TypeParameter* param) {
    assert(has(param));
    return param->index() < args.size() ? args[param->index()] : nullptr;
  }

  void Env::fromSpecialization(SpecializedDefn* sd) {
    args.assign(sd->typeArgs().begin(), sd->typeArgs().end());
    params = sd->typeParams();
  }

  Member* Env::unwrap(Member* m) {
    if (auto sp = dyn_cast<SpecializedDefn>(m)) {
      fromSpecialization(sp);
      return sp->generic();
    }
    return m;
  }
}
