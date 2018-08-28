#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
#define TEMPEST_SEMA_INFER_TYPES_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
  #include "tempest/sema/infer/constraint.hpp"
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  class OverloadCandidate;
  class ConstraintSolver;

  /** A contingent type is a set of possible types, only one of which will eventually be chosen.
      The type to be chosen will depend on which call candidate gets selected. */
  class ContingentType : public Type {
  public:
    struct Entry {
      const OverloadCandidate* when;
      const Type* type;
    };

    ContingentType(llvm::ArrayRef<Entry> entries)
      : Type(Kind::CONTINGENT)
      , entries(entries)
    {}

    /** List of contingent entries including their predicates. */
    llvm::ArrayRef<Entry> entries;
  };

  /** A type which needs to be inferred. */
  class InferredType : public Type {
  public:
    InferredType(
        TypeParameter* typeParam,
        ConstraintSolver* solver,
        const OverloadCandidate* context)
      : Type(Kind::INFERRED)
      , typeParam(typeParam)
      , solver(solver)
      , context(context)
    {}

    /** The original type variable. */
    TypeParameter* typeParam;

    /** The unique index of this type. */
    size_t index = 0;

    /** Reference to the constraint solver. */
    ConstraintSolver* solver;

    /** The candidate which defined this type variable. */
    const OverloadCandidate* context;
  };
}

#endif
