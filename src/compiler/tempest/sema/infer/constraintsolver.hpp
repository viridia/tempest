#ifndef TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP
#define TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP 1

#ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
  #include "tempest/sema/infer/constraint.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_OVERLOAD_HPP
  #include "tempest/sema/infer/overload.hpp"
#endif

namespace tempest::sema::graph {
  class GenericDefn;
}

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;

  /** Base class for constraints to be solved. */
  class ConstraintSolver {
  public:
    source::Location location;

    ConstraintSolver(
        const source::Location& location
        // GenericDefn* enclosingGeneric
        )
      : location(location)
      // , _enclosingGeneric(enclosingGeneric)
    {}

  void addAssignment(
      const source::Location& loc,
      Type* dstType,
      Expr* srcExpr,
      llvm::ArrayRef<OverloadCandidate*> when);
  void addBindingConstraint(
      const source::Location& loc,
      Type* dstType,
      Type* srcType,
      BindingConstraint::Restriction restriction,
      llvm::ArrayRef<OverloadCandidate*> when);

  private:
    llvm::BumpPtrAllocator _alloc;
    // GenericDefn* _enclosingGeneric;
    std::vector<ConversionConstraint*> _conversionConstraints;
    std::vector<BindingConstraint*> _bindingConstraints;
    std::vector<OverloadSite*> _sites;
  };
}

#endif
