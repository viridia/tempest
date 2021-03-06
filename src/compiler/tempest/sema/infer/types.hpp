#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
#define TEMPEST_SEMA_INFER_TYPES_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_CONDITIONS_HPP
  #include "tempest/sema/infer/conditions.hpp"
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  class OverloadCandidate;
  class ConstraintSolver;

  /** A testable relation between two types. */
  enum class TypeRelation {
    EQUAL,
    SUBTYPE,
    SUPERTYPE,
    ASSIGNABLE_FROM,
    ASSIGNABLE_TO,
  };

  inline TypeRelation inverseRelation(TypeRelation predicate) {
    switch (predicate) {
      case TypeRelation::EQUAL: return TypeRelation::EQUAL;
      case TypeRelation::SUBTYPE: return TypeRelation::SUPERTYPE;
      case TypeRelation::SUPERTYPE: return TypeRelation::SUBTYPE;
      case TypeRelation::ASSIGNABLE_FROM: return TypeRelation::ASSIGNABLE_TO;
      case TypeRelation::ASSIGNABLE_TO: return TypeRelation::ASSIGNABLE_FROM;
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
    // Potential choices for the value of this inferred type.
    struct Constraint {
      const Type* value;
      TypeRelation predicate;
      Conditions when;

      Constraint(
        const Type* value,
        TypeRelation predicate,
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

  /** Wrapper which tells formatter to expand to the set of constraints. */
  struct ShowConstraints {
    const Type* type;
    ShowConstraints(const Type* type) : type(type) {};
  };

  ::std::ostream& operator<<(::std::ostream& os, const ShowConstraints& sc);
}

#endif
