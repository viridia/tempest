#ifndef TEMPEST_SEMA_GRAPH_EXPR_STMT_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_STMT_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

namespace tempest::sema::graph {
  using llvm::ArrayRef;

  class ValueDefn;

  /** A statement block. */
  class BlockStmt : public Expr {
  public:
    ArrayRef<Expr*> stmts;
    Expr* result;

    BlockStmt(Location location, const ArrayRef<Expr*>& stmts, Expr* result = nullptr,
        Type* type = nullptr)
      : Expr(Kind::BLOCK, location, type)
      , stmts(stmts)
      , result(result)
    {}

    /** Dynamic casting support. */
    static bool classof(const BlockStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::BLOCK; }
  };

  /** A throw statemtn. */
  class LocalVarStmt : public Expr {
  public:
    ValueDefn* defn;

    LocalVarStmt(Location location, ValueDefn* defn)
      : Expr(Kind::LOCAL_VAR, location)
      , defn(defn)
    {}

    /** Dynamic casting support. */
    static bool classof(const LocalVarStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::LOCAL_VAR; }
  };

  /** An if-statement. */
  class IfStmt : public Expr {
  public:
    /** The test expression. */
    Expr* test;

    /** The 'then' block. */
    Expr* thenBlock;

    /** The 'else' block. */
    Expr* elseBlock;

    IfStmt(Location location, Expr* test, Expr* thenBlock, Expr* elseBlock, Type* type = nullptr)
      : Expr(Kind::IF, location, type)
      , test(test)
      , thenBlock(thenBlock)
      , elseBlock(elseBlock)
    {}

    /** Dynamic casting support. */
    static bool classof(const IfStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::IF; }
  };

  /** A while-statement. */
  class WhileStmt : public Expr {
  public:
    Expr* test;
    Expr* body;

    WhileStmt(Location location, Expr* test, Expr* body, Type* type = nullptr)
      : Expr(Kind::WHILE, location, type)
      , test(test)
      , body(body)
    {}

    /** Dynamic casting support. */
    static bool classof(const WhileStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::WHILE; }
  };

  #if 0
  struct Loop(Expr) = ExprType.LOOP {
    body: Expr = 2;
  }
  #endif

  /** A for-statement. */
  // class ForStmt : public Expr {
  // public:
  //   ForStmt(Location location)
  //     : Expr(Kind::FOR, location)
  //     , _init(nullptr)
  //     , _test(nullptr)
  //     , _step(nullptr)
  //     , _body(nullptr)
  //   {}

  //   /** The list of defined variables. */
  //   const ArrayRef<ValueDefn*>& vars() const { return _vars; }
  //   void setVars(const ArrayRef<ValueDefn*>& vars) { _vars = vars; }

  //   /** The init expression. */
  //   Expr* init() const { return _init; }
  //   void setInit(Expr* e) { _init = e; }

  //   /** The test expression. */
  //   Expr* test() const { return _test; }
  //   void setTest(Expr* e) { _test = e; }

  //   /** The step expression. */
  //   Expr* step() const { return _step; }
  //   void setStep(Expr* e) { _step = e; }

  //   /** The loop body. */
  //   Expr* body() const { return _body; }
  //   void setBody(Expr* e) { _body = e; }

  //   /** Dynamic casting support. */
  //   static bool classof(const ForStmt* e) { return true; }
  //   static bool classof(const Expr* e) { return e->kind == Kind::FOR; }
  // private:
  //   ArrayRef<ValueDefn*> _vars;
  //   Expr* _init;
  //   Expr* _test;
  //   Expr* _step;
  //   Expr* _body;
  // };

  /** A for-in-statement. */
  class ForInStmt : public Expr {
  public:
    ArrayRef<ValueDefn*> vars;
    Expr* iter;
    Expr* body;
    ValueDefn* iterVar;
    ValueDefn* nextVar;

    ForInStmt(
        Location location,
        Expr* iter = nullptr,
        Expr* body = nullptr,
        ValueDefn* iterVar = nullptr,
        ValueDefn* nextVar = nullptr)
      : Expr(Kind::FOR_IN, location)
      , iter(iter)
      , body(body)
      , iterVar(iterVar)
      , nextVar(nullptr)
    {}

    /** Dynamic casting support. */
    static bool classof(const ForInStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::FOR_IN; }
  };

  /** A switch case statement. */
  class CaseStmt : public Expr {
  public:
    ArrayRef<Expr*> values;
    ArrayRef<Expr*> rangeValues;
    Expr* body;

    CaseStmt(Location location)
      : Expr(Kind::SWITCH_CASE, location)
      , body(nullptr)
    {}

    /** Dynamic casting support. */
    static bool classof(const CaseStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SWITCH_CASE; }
  };

  /** A switch statement. */
  class SwitchStmt : public Expr {
  public:
    Expr* testExpr;
    ArrayRef<CaseStmt*> cases;
    Expr* elseBody;

    SwitchStmt(Location location)
      : Expr(Kind::SWITCH, location)
      , testExpr(nullptr)
      , elseBody(nullptr)
    {}

    /** Dynamic casting support. */
    static bool classof(const SwitchStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SWITCH; }
  };

  /** A match pattern. */
  class Pattern : public Expr {
  public:
    ValueDefn* var;
    Expr* body;

    Pattern(Location location)
      : Expr(Kind::MATCH_PATTERN, location)
      , var(nullptr)
      , body(nullptr)
    {}

    /** Dynamic casting support. */
    static bool classof(const Pattern* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::MATCH_PATTERN; }
  };

  /** A match statement. */
  class MatchStmt : public Expr {
  public:
    Expr* testExpr;
    ArrayRef<Pattern*> patterns;
    Expr* elseBody;

    MatchStmt(Location location)
      : Expr(Kind::MATCH, location)
      , testExpr(nullptr)
      , elseBody(nullptr)
    {}

    /** Dynamic casting support. */
    static bool classof(const MatchStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::MATCH; }
  };

  /** A return statemtn. */
  class ReturnStmt : public Expr {
  public:
    Expr* expr;

    ReturnStmt(Location location, Expr* expr = nullptr)
      : Expr(Kind::RETURN, location)
      , expr(expr)
    {}
    ReturnStmt(Expr* expr) : Expr(Kind::RETURN, Location()), expr(expr) {}

    /** Dynamic casting support. */
    static bool classof(const ReturnStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::RETURN; }
  };

  /** A throw statemtn. */
  class ThrowStmt : public Expr {
  public:
    Expr* expr;

    ThrowStmt(Location location, Expr* expr = nullptr)
      : Expr(Kind::THROW, location)
      , expr(expr)
    {}

    /** Dynamic casting support. */
    static bool classof(const ThrowStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::THROW; }
  };

  #if 0
  struct Try(Expr) = ExprType.TRY {
    struct Catch {
      location: spark.location.SourceLocation = 1;
      exceptDefn: defn.Var = 2;
      body: Expr = 3;
    }
    body: Expr = 1;
    catchList: list[Catch] = 2;
    else: Expr = 3;
    finally: Expr = 4;
  }

  struct Break(Oper) = ExprType.BREAK {}
  struct Continue(Oper) = ExprType.CONTINUE {}
  struct Unreachable(Oper) = ExprType.UNREACHABLE {}

  struct LocalDefn(Expr) = ExprType.LOCAL_DECL {
    defn: defn.Defn = 1;
  }
  struct Intrinsic(Expr) = ExprType.INTRINSIC {}
  #endif

}

#endif

