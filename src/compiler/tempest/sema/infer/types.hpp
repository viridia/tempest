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

  /** A contingent type is a set of possible types, only one of which will eventually be chosen.
      The type to be chosen will depend on which call candidate gets selected. */
  class ContingentType : public Type {
  public:
    struct Entry {
      const OverloadCandidate* when;
      Type* type;
    };

    ContingentType(llvm::ArrayRef<Entry> entries)
      : Type(Kind::CONTINGENT)
      , entries(entries)
    {}

    /** List of contingent entries including their predicates. */
    llvm::ArrayRef<Entry> entries;
  };

  /** A type variable which has been renamed so that each call site has distinct vars. */
  class RenamedTypeVar : public Type {
  public:
    RenamedTypeVar(TypeVar* original)
      : Type(Kind::RENAMED)
      , original(original)
    {}

    /** The original type variable. */
    TypeVar* original;

    /** The unique index of this type. */
    size_t index = 0;
  };
}

#endif
