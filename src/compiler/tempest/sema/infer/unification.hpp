#ifndef TEMPEST_SEMA_INFER_UNIFICATION_HPP
#define TEMPEST_SEMA_INFER_UNIFICATION_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_CONDITIONS_HPP
  #include "tempest/sema/infer/conditions.hpp"
#endif

namespace tempest::support {
  class BumpPtrAllocator;
}

namespace tempest::sema::graph {
  struct Env;
  class TypeParameter;
}

namespace tempest::sema::infer {
  using tempest::sema::convert::Env;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeParameter;

  class InferredType;

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

  struct UnificationResult {
    UnificationResult(const UnificationResult& src) = default;
    UnificationResult(UnificationResult&& src) = default;

    UnificationResult& operator=(const UnificationResult& src) {
      param = src.param;
      value = src.value;
      predicate = src.predicate;
      conditions = src.conditions;
      return *this;
    }

    /** The type parameter that we are binding a result to. */
    InferredType* param;

    /** The value being bound to the type. */
    const Type* value;

    /** What kind of binding constraint. */
    BindingPredicate predicate;

    /** When is this binding relevant. */
    Conditions conditions;
  };

  /** Unification algorithm.
      Guarantee: `result` will be empty if unification fails. This is very important because
      unify calls itself recursively and builds up the result upon successful unifications. */
  bool unify(
    std::vector<UnificationResult>& result, const Type* lt, const Type* rt, Conditions& when,
    BindingPredicate predicate, tempest::support::BumpPtrAllocator& alloc);
  bool unify(
    std::vector<UnificationResult>& result,
    const Type* lt, Env& ltEnv,
    const Type* rt, Env& rtEnv,
    Conditions& when,
    BindingPredicate predicate,
    tempest::support::BumpPtrAllocator& alloc);
}

#endif
