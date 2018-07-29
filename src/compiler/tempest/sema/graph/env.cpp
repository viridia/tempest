#include "tempest/sema/graph/env.hpp"

namespace tempest::sema::graph {
  static ImmutableEnv emptyEnv;
  EnvChain EnvChain::NIL(emptyEnv);
}
