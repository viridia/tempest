#include "tempest/sema/graph/defn.h"
#include "tempest/sema/graph/type.h"

namespace tempest::sema::graph {
    /** The ordinal index of this type variable relative to other type variables. */
  int32_t TypeVar::index() const { 
    return param->index();
  }
}
