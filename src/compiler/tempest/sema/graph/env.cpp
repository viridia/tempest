#include "tempest/sema/graph/env.h"

namespace tempest::sema::graph {
  static ImmutableEnv emptyEnv;
  EnvChain EnvChain::NIL(emptyEnv);
}
