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
    Expr* stem = nullptr;
    SymbolRefExpr(
          Kind kind,
          Location location,
          gen::OutputSym* sym,
          const Type* type = nullptr,
          Expr* stem = nullptr)
      : Expr(kind, location, type)
      , sym(sym)
      , stem(stem)
    {}

    /** Dynamic casting support. */
    static bool classof(const SymbolRefExpr* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::GLOBAL_REF || e->kind == Kind::ALLOC_OBJ;
    }
  };

  /** A call to allocate a variable-length class. */
  class FlexAllocExpr : public Expr {
  public:
    gen::OutputSym* cls;
    Expr* size = nullptr;
    FlexAllocExpr(
          Location location,
          gen::OutputSym* cls,
          Expr* size,
          const Type* type = nullptr)
      : Expr(Kind::ALLOC_FLEX, location, type)
      , cls(cls)
      , size(size)
    {}

    /** Dynamic casting support. */
    static bool classof(const FlexAllocExpr* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::ALLOC_FLEX; }
  };
}

#endif
