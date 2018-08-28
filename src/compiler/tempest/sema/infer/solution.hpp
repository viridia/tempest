#ifndef TEMPEST_SEMA_INFER_SOLUTION_HPP
#define TEMPEST_SEMA_INFER_SOLUTION_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
  #include "tempest/sema/graph/transform.hpp"
#endif

namespace tempest::sema::infer {
  class ConstraintSolver;

  /** Applies the type inference solution to type expressions. */
  class SolutionTransform : public TypeTransform {
  public:
    SolutionTransform(llvm::BumpPtrAllocator& alloc, ConstraintSolver& cs)
      : TypeTransform(alloc)
      , _cs(cs)
    {}
    SolutionTransform(const SolutionTransform&) = delete;

    void buildSolutionMap();

    const Type* transformInferredType(const InferredType* ivar);
    const Type* transformContingentType(const ContingentType* contingent);

  private:
    ConstraintSolver& _cs;
    std::unordered_map<const InferredType*, Type*> _solutionMap;
  };
}

#endif
