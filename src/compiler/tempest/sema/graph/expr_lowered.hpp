#ifndef TEMPEST_SEMA_GRAPH_EXPR_LOWERED_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_LOWERED_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

namespace tempest::gen {
  class OutputSym;
}

namespace tempest::sema::graph {
  /** A reference to a generated symbol such as a class descriptor table. */
  class SymbolRefExpr : public Expr {
  public:
    gen::OutputSym* sym;
    SymbolRefExpr(
          Kind kind,
          Location location,
          gen::OutputSym* sym,
          Type* type = nullptr)
      : Expr(kind, location, type)
      , sym(sym)
    {}
  };
}

#endif
