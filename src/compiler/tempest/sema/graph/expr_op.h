#ifndef TEMPEST_SEMA_GRAPH_EXPR_OP_H
#define TEMPEST_SEMA_GRAPH_EXPR_OP_H 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_H
  #include "tempest/sema/graph/expr.h"
#endif

namespace tempest::sema::graph {

  /** Infix operator. */
  class InfixOp : public Expr {
  public:
    Expr* lhs;
    Expr* rhs;
    Type* type;

    InfixOp(
          Kind kind,
          Location location,
          Expr* lhs,
          Expr* rhs,
          Type* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, location)
      , lhs(lhs)
      , rhs(rhs)
      , type(type)
    {}
    InfixOp(Kind kind, Expr* lhs, Expr* rhs, Type* type = nullptr)
      : Expr(kind, Location())
      , lhs(lhs)
      , rhs(rhs)
      , type(type)
    {}

    /** Dynamic casting support. */
    static bool classof(const InfixOp* e) { return true; }
    static bool classof(const Expr* e) { 
      return e->kind >= Kind::INFIX_START && e->kind <= Kind::INFIX_END;
    }
  };

  /** Relational operator. */
  class RelationalOp : public Expr {
  public:
    Expr* lhs;
    Expr* rhs;
    Type* type;

    RelationalOp(
          Kind kind,
          Location location,
          Expr* lhs,
          Expr* rhs,
          Type* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, location)
      , lhs(lhs)
      , rhs(rhs)
      , type(type)
    {}
    RelationalOp(Kind kind, Expr* lhs, Expr* rhs, Type* type = nullptr)
      : Expr(kind, Location())
      , lhs(lhs)
      , rhs(rhs)
      , type(type)
    {}

    /** Dynamic casting support. */
    static bool classof(const InfixOp* e) { return true; }
    static bool classof(const Expr* e) { 
      return e->kind >= Kind::RELATIONAL_START && e->kind <= Kind::RELATIONAL_END;
    }
  };
}

#endif
