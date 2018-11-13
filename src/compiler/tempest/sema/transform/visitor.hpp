#ifndef TEMPEST_SEMA_GRAPH_VISITOR_HPP
#define TEMPEST_SEMA_GRAPH_VISITOR_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

namespace tempest::sema::transform {
  using tempest::sema::graph::Type;
  using tempest::sema::graph::Expr;

  /** Abstract base class for mutating expression transformations. */
  class ExprVisitor {
  public:
    Expr* operator()(Expr* in) { return visit(in); }

    virtual Expr* visit(Expr* in);
    void visitArray(const ArrayRef<Expr*>& in);
  };
}

#endif
