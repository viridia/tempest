#include "tempest/sema/infer/constraintsolver.hpp"

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;

  void ConstraintSolver::addAssignment(
      const source::Location& loc,
      Type* dstType,
      Expr* srcExpr,
      llvm::ArrayRef<OverloadCandidate*> when) {
    (void)_alloc;
    // (void)_enclosingGeneric;
  }

  void ConstraintSolver::addBindingConstraint(
      const source::Location& loc,
      Type* dstType,
      Type* srcType,
      BindingConstraint::Restriction restriction,
      llvm::ArrayRef<OverloadCandidate*> when) {

  }
}
