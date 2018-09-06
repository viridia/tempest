#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/types.hpp"

namespace tempest::sema::infer {
  bool InferredType::isViable(const Constraint& c) const {
    return solver->isViable(c.when);
  }
}
