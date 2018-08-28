#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/solution.hpp"

namespace tempest::sema::infer {

  void SolutionTransform::buildSolutionMap() {
    // TODO:
    (void)_cs;
  }

  const Type* SolutionTransform::transformInferredType(const InferredType* ivar) {
    auto it = _solutionMap.find(ivar);
    assert(it != _solutionMap.end());
    return it->second;
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
