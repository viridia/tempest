#ifndef TEMPEST_SEMA_GRAPH_TYPEORDER_HPP
#define TEMPEST_SEMA_GRAPH_TYPEORDER_HPP 1

namespace tempest::sema::graph {
  class Type;
  class Member;

  /** Defines a total order of type expressions. Used to canonically sort types. */
  struct TypeOrder {
    bool operator()(const Type* lhs, const Type* rhs) const;
    bool memberOrder(const Member* lhs, const Member* rhs) const;
  };
}

#endif
