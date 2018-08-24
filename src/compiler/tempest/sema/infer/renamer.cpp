#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/infer/renamer.hpp"

namespace tempest::sema::infer {
  const Type* TypeVarRenamer::transformTypeVar(const TypeVar* tvar) {
    // TODO: Make sure we only rename type vars that are part of the curent context!
    assert(false && "Implement");
  }
}
