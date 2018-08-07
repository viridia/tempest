#include "tempest/sema/graph/expr.hpp"

namespace tempest::sema::graph {
  Expr Expr::ERROR(Expr::Kind::INVALID, Location());
}

