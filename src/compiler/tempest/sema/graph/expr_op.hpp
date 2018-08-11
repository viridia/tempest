#ifndef TEMPEST_SEMA_GRAPH_EXPR_OP_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_OP_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

namespace tempest::sema::graph {

  /** Unary operator. */
  class UnaryOp : public Expr {
  public:
    Expr* arg;
    Type* type;

    UnaryOp(Kind kind, Location location, Expr* arg, Type* type = nullptr)
      : Expr(kind, location)
      , arg(arg)
      , type(type)
    {}
    UnaryOp(Kind kind, Expr* arg, Type* type = nullptr)
      : Expr(kind, Location())
      , arg(arg)
      , type(type)
    {}

    /** Dynamic casting support. */
    static bool classof(const UnaryOp* e) { return true; }
    static bool classof(const Expr* e) {
      switch (e->kind) {
        case Kind::NOT:
          return true;
        default:
          return false;
      }
    }
  };

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
      : Expr(kind, location)
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
      : Expr(kind, location)
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

  /** Operator with a variable number of arguments. */
  class InvokeOp : public Expr {
  public:
    Expr* function;
    llvm::ArrayRef<Expr*> args;
    Type* type;

    InvokeOp(
          Kind kind,
          Location location,
          Expr* function,
          llvm::ArrayRef<Expr*> args,
          Type* type = nullptr)
      : Expr(kind, location)
      , function(function)
      , args(args)
      , type(type)
    {}

    /** Dynamic casting support. */
    static bool classof(const InfixOp* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::CALL;
    }
  };
}

#endif
