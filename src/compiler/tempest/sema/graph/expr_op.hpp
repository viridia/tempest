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

    UnaryOp(Kind kind, Location location, Expr* arg, Type* type = nullptr)
      : Expr(kind, location, type)
      , arg(arg)
    {}
    UnaryOp(Kind kind, Expr* arg, Type* type = nullptr)
      : Expr(kind, Location(), type)
      , arg(arg)
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
  class BinaryOp : public Expr {
  public:
    Expr* lhs;
    Expr* rhs;

    BinaryOp(
          Kind kind,
          Location location,
          Expr* lhs,
          Expr* rhs,
          Type* type = nullptr)
      : Expr(kind, location, type)
      , lhs(lhs)
      , rhs(rhs)
    {}
    BinaryOp(Kind kind, Expr* lhs, Expr* rhs, Type* type = nullptr)
      : Expr(kind, Location(), type)
      , lhs(lhs)
      , rhs(rhs)
    {}

    /** Dynamic casting support. */
    static bool classof(const BinaryOp* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind >= Kind::INFIX_START && e->kind <= Kind::INFIX_END;
    }
  };

  /** Operator with a variable number of arguments. */
  class MultiOp : public Expr {
  public:
    llvm::ArrayRef<Expr*> args;

    MultiOp(
          Kind kind,
          Location location,
          llvm::ArrayRef<Expr*> args,
          Type* type = nullptr)
      : Expr(kind, location, type)
      , args(args)
    {}

    /** Dynamic casting support. */
    static bool classof(const BinaryOp* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::REST_ARGS || e->kind == Kind::ARRAY_LITERAL;
    }
  };

  /** Operator with a variable number of arguments. */
  class ApplyFnOp : public Expr {
  public:
    Expr* function;
    llvm::ArrayRef<Expr*> args;

    ApplyFnOp(
          Kind kind,
          Location location,
          Expr* function,
          llvm::ArrayRef<Expr*> args,
          Type* type = nullptr)
      : Expr(kind, location, type)
      , function(function)
      , args(args)
    {}

    /** Dynamic casting support. */
    static bool classof(const BinaryOp* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::CALL;
    }
  };
}

#endif
