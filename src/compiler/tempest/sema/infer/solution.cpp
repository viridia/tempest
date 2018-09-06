#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/solution.hpp"

namespace tempest::sema::infer {

  const Type* SolutionTransform::transformInferredType(const InferredType* ivar) {
    assert(ivar->value);
    return ivar->value;
  }

  const Type* SolutionTransform::transformContingentType(const ContingentType* contingent) {
    for (auto entry : contingent->entries) {
      if (entry.when->isViable()) {
        return transform(entry.type);
      }
    }
    assert(false && "Contingent type had no viable choices");
  }
}
