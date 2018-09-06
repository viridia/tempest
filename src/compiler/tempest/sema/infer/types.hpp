#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
#define TEMPEST_SEMA_INFER_TYPES_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

// #ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
//   #include "tempest/sema/infer/constraint.hpp"
// #endif

#ifndef TEMPEST_SEMA_INFER_CONDITIONS_HPP
  #include "tempest/sema/infer/conditions.hpp"
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  class OverloadCandidate;
  class ConstraintSolver;

  enum class BindingPredicate {
    EQUAL,
    SUBTYPE,
    SUPERTYPE,
    ASSIGNABLE_FROM,
    ASSIGNABLE_TO,
  };

  inline BindingPredicate inversePredicate(BindingPredicate predicate) {
    switch (predicate) {
      case BindingPredicate::EQUAL: return BindingPredicate::EQUAL;
      case BindingPredicate::SUBTYPE: return BindingPredicate::SUPERTYPE;
      case BindingPredicate::SUPERTYPE: return BindingPredicate::SUBTYPE;
      case BindingPredicate::ASSIGNABLE_FROM: return BindingPredicate::ASSIGNABLE_TO;
      case BindingPredicate::ASSIGNABLE_TO: return BindingPredicate::ASSIGNABLE_FROM;
    }
  }

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

    /** Dynamic casting support. */
    static bool classof(const ContingentType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::CONTINGENT; }
  };

  /** A type which needs to be inferred. */
  class InferredType : public Type {
  public:
    struct Constraint {
      const Type* value;
      BindingPredicate predicate;
      Conditions when;

      Constraint(
        const Type* value,
        BindingPredicate predicate,
        Conditions when)
        : value(value)
        , predicate(predicate)
        , when(when)
      {}
      Constraint(const Constraint& src)
        : value(src.value)
        , predicate(src.predicate)
        , when(src.when)
      {}
      Constraint(Constraint&& src)
        : value(src.value)
        , predicate(src.predicate)
        , when(std::move(src.when))
      {}
      Constraint& operator=(const Constraint& src) {
        value = src.value;
        predicate = src.predicate;
        when = src.when;
        return *this;
      }
      Constraint& operator=(Constraint&& src) {
        value = src.value;
        predicate = src.predicate;
        when = std::move(src.when);
        return *this;
      }
    };

    InferredType(
        TypeParameter* typeParam,
        ConstraintSolver* solver)
      : Type(Kind::INFERRED)
      , typeParam(typeParam)
      , solver(solver)
    {}

    /** The original type variable. */
    TypeParameter* typeParam;

    /** The unique index of this type. */
    size_t index = 0;

    /** Reference to the solver instance. */
    ConstraintSolver* solver;

    /** Set of constraints on this type, must be reconciled during constrant solving. */
    mutable llvm::SmallVector<Constraint, 4> constraints;

    /** The final value assigned to this inferred type. */
    mutable const Type* value = nullptr;

    bool isViable(const Constraint&) const;

    /** Dynamic casting support. */
    static bool classof(const InferredType* t) { return true; }
    static bool classof(const Type* t) { return t->kind == Kind::INFERRED; }
  };
}

#endif
