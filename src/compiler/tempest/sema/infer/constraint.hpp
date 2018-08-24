#ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
#define TEMPEST_SEMA_INFER_CONSTRAINT_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

namespace tempest::sema::infer {
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeVar;
  class OverloadCandidate;

  /** Base class for constraints to be solved. */
  class Constraint {
  public:
    /** The conditions under which this constraint holds. */
    llvm::ArrayRef<const OverloadCandidate*> when;

    /** The original when-list, for diagnostic purposes. */
    llvm::ArrayRef<const OverloadCandidate*> originalWhen;

    bool valid = true;
  };

  /** Constraint representing an assignment of a value from one type to another. */
  class ConversionConstraint : public Constraint {
  public:
    /** The source expression being assigned. */
    Expr* srcExpr;

    /** The type of the source expression. */
    Type* srcType;

    /** The type of whatever it's being assigned to. */
    Type* dstType;
  };

  /** Constraint representing whether a value can be bound to a given type parameter.
      This includes subclass constraints and trait constraints.
  */
  class BindingConstraint : public Constraint {
  public:
    enum Restriction {
      SUBTYPE,    // Source type must be a subtype (covariant)
      EQUAL,      // Types must be equal (invariant)
      SUPERTYPE,  // Source tupe must be a supertype (contravariant)
    };

    /** The type of the source expression. */
    Type* srcType;

    /** The type of the source expression. */
    Type* dstType;

    /** The type variable it's being bound to. */
    TypeVar* typeVar;

    /** What kind of restriction. */
    Restriction restriction;
  };
}

#endif
