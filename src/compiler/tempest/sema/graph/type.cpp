#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"

namespace tempest::sema::graph {

  Type Type::ERROR(Type::Kind::INVALID);

    /** The ordinal index of this type variable relative to other type variables. */
  int32_t TypeVar::index() const {
    return param->index();
  }
}
