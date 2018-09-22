#ifndef TEMPEST_SEMA_INFER_SOLUTION_HPP
#define TEMPEST_SEMA_INFER_SOLUTION_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
  #include "tempest/sema/transform/transform.hpp"
#endif

namespace tempest::sema::infer {
  class ConstraintSolver;

  /** Applies the type inference solution to type expressions. */
  class SolutionTransform : public transform::TypeTransform {
  public:
    SolutionTransform(tempest::support::BumpPtrAllocator& alloc)
      : transform::TypeTransform(alloc)
    {}
    SolutionTransform(const SolutionTransform&) = delete;

    const Type* transformInferredType(const InferredType* ivar);
    const Type* transformContingentType(const ContingentType* contingent);

  private:
    std::unordered_map<const InferredType*, Type*> _solutionMap;
  };
}

#endif
