#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/env.hpp"
#include "tempest/sema/graph/type.hpp"

namespace tempest::sema::graph {

  /** True if this type variable is included in the environment. */
  bool Env::has(const TypeVar* tvar) {
    return has(tvar->param);
  }

  /** True if this type parameter is included in the environment. */
  bool Env::has(const TypeParameter* param) {
    return std::find(params.begin(), params.end(), param) != params.end();
  }

  /** Return the mapped value of this type parameter. */
  const Type* Env::get(const TypeVar* tvar) {
    return get(tvar->param);
  }

  const Type* Env::get(const TypeParameter* param) {
    assert(has(param));
    return param->index() < args.size() ? args[param->index()] : nullptr;
  }
#if 0
  static ImmutableEnv emptyEnv;
  EnvChain EnvChain::NIL(emptyEnv);
#endif
}
