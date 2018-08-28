#ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
#define TEMPEST_SEMA_INFER_CONSTRAINT_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_CONDITIONS_HPP
  #include "tempest/sema/infer/conditions.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#include <algorithm>

namespace tempest::sema::infer {
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeVar;
  /** Base class for constraints to be solved. */
  class Constraint {
  public:
    /** The conditions under which this constraint holds. */
    Conditions when;

    /** The original when-list, for diagnostic purposes. */
    Conditions originalWhen;

    bool valid = true;

    Constraint() {}
    Constraint(Conditions when) : when(std::move(when)), originalWhen(when) {}
  };

  /** Constraint representing an assignment of a value from one type to another. */
  class ConversionConstraint : public Constraint {
  public:
    /** The type of whatever it's being assigned to. */
    const Type* dstType;

    /** The type of the source expression. */
    const Type* srcType;

    ConversionConstraint(Conditions when, const Type* dstType, const Type* srcType)
      : Constraint(when)
      , dstType(dstType)
      , srcType(srcType)
    {}
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
    const Type* dstType;

    /** The type of the source expression. */
    const Type* srcType;

    /** The type variable it's being bound to. */
    const TypeVar* typeVar;

    /** What kind of restriction. */
    Restriction restriction;

    BindingConstraint(
        Conditions when,
        const Type* dstType,
        const Type* srcType,
        const TypeVar* typeVar,
        Restriction restriction)
      : Constraint(when)
      , dstType(dstType)
      , srcType(srcType)
      , typeVar(typeVar)
      , restriction(restriction)
    {}
  };
}

#endif
