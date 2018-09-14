#ifndef TEMPEST_SEMA_INFER_REJECTION_HPP
#define TEMPEST_SEMA_INFER_REJECTION_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  struct ExplicitConstraint;

  /** During type inference, it's important to keep track of why a given candidate was
      rejected so that meaningful diagnostics can be shown to the user. */
  struct Rejection {
    enum Reason {
      // Not rejected
      NONE,

      // Parameter assignment rejections
      NOT_ENOUGH_ARGS,
      TOO_MANY_ARGS,
      KEYWORD_NOT_FOUND,
      KEYWORD_IN_USE,
      KEYWORD_ONLY_ARG,

      // Type inference rejections
      UNIFICATION_ERROR,
      CONVERSION_FAILURE,
      QUALIFIER_LOSS,
      UNSATISFIED_REQIREMENT,
      UNSATISFIED_TYPE_CONSTRAINT,
      INCONSISTENT, // Contradictory constraints
      CONFLICT, // Conflict between explicit and implicit constraint
      NOT_MORE_SPECIALIZED,
      NOT_BEST,
    };

    /** Reason for the rejection. */
    Reason reason;

    size_t argIndex = 0;

    /** Some rejections include a constraint that wasn't satisfied. */
    ExplicitConstraint* constraint = nullptr;

    /** Some rejections include a constraint that wasn't satisfied. */
    const InferredType* itype = nullptr;
    InferredType::Constraint* iconst0 = nullptr;
    InferredType::Constraint* iconst1 = nullptr;

    Rejection() : reason(NONE) {};
    Rejection(Reason reason) : reason(reason) {};
    Rejection(Reason reason, size_t argIndex) : reason(reason), argIndex(argIndex) {};
    Rejection(const Rejection&) = default;
  };
}

#endif
