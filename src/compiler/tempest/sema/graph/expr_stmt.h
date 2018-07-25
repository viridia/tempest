#ifndef TEMPEST_SEMA_GRAPH_EXPR_STMT_H
#define TEMPEST_SEMA_GRAPH_EXPR_STMT_H 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_H
  #include "tempest/sema/graph/expr.h"
#endif

namespace tempest::sema::graph {
  using llvm::ArrayRef;

  class ValueDefn;

  /** A statement block. */
  class BlockStmt : public Expr {
  public:
    BlockStmt(Location location, const ArrayRef<Expr*>& stmts, Expr* result = nullptr)
      : Expr(Kind::BLOCK, location)
      , _stmts(stmts)
      , _result(result)
    {}

    /** The list of stmts to assign to. */
    const ArrayRef<Expr*>& stmts() const { return _stmts; }
    void setStmts(const ArrayRef<Expr*>& stmts) { _stmts = stmts; }

    /** The result expression of the block. */
    Expr* result() const { return _result; }
    void setResult(Expr* result) { _result = result; }

    /** Dynamic casting support. */
    static bool classof(const BlockStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::BLOCK; }
  private:
    ArrayRef<Expr*> _stmts;
    Expr* _result;
  };

  /** An if-statement. */
  class IfStmt : public Expr {
  public:
    IfStmt(Location location, Expr* test, Expr* thenBlock, Expr* elseBlock)
      : Expr(Kind::IF, location)
      , _test(test)
      , _thenBlock(thenBlock)
      , _elseBlock(elseBlock)
    {}

    /** The test expression. */
    Expr* test() const { return _test; }

    /** The 'then' block. */
    Expr* thenBlock() const { return _thenBlock; }

    /** The 'else' block. */
    Expr* elseBlock() const { return _elseBlock; }

    /** Dynamic casting support. */
    static bool classof(const IfStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::IF; }
  private:
    Expr* _test;
    Expr* _thenBlock;
    Expr* _elseBlock;
  };

  /** A while-statement. */
  class WhileStmt : public Expr {
  public:
    WhileStmt(Location location, Expr* test, Expr* body)
      : Expr(Kind::WHILE, location)
      , _test(test)
      , _body(body)
    {}

    /** The test expression. */
    Expr* test() const { return _test; }

    /** The 'then' block. */
    Expr* body() const { return _body; }

    /** Dynamic casting support. */
    static bool classof(const WhileStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::WHILE; }
  private:
    Expr* _test;
    Expr* _body;
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
    ForInStmt(
        Location location,
        Expr* iter = nullptr,
        Expr* body = nullptr,
        ValueDefn* iterVar = nullptr,
        ValueDefn* nextVar = nullptr)
      : Expr(Kind::FOR_IN, location)
      , _iter(iter)
      , _body(body)
      , _iterVar(iterVar)
      , _nextVar(nullptr)
    {}

    /** The list of defined variables. */
    const ArrayRef<ValueDefn*>& vars() const { return _vars; }
    void setVars(const ArrayRef<ValueDefn*>& vars) { _vars = vars; }

    /** The init expression. */
    Expr* iter() const { return _iter; }
    void setIter(Expr* e) { _iter = e; }

    /** The loop body. */
    Expr* body() const { return _body; }
    void setBody(Expr* e) { _body = e; }

    /** The variable that contains the iteration expression. */
    ValueDefn* iterVar() const { return _iterVar; }
    void setIterVar(ValueDefn* vd) { _iterVar = vd; }

    /** The variable that contains the value of iterator.next(). This is used to determine
        if we've reached the end of the sequence before assigning the values to the individual
        loop vars. */
    ValueDefn* nextVar() const { return _nextVar; }
    void setNextVar(ValueDefn* vd) { _nextVar = vd; }

    /** Dynamic casting support. */
    static bool classof(const ForInStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::FOR_IN; }
  private:
    ArrayRef<ValueDefn*> _vars;
    Expr* _iter;
    Expr* _body;
    ValueDefn* _iterVar;
    ValueDefn* _nextVar;
  };

  /** A switch case statement. */
  class CaseStmt : public Expr {
  public:
    CaseStmt(Location location)
      : Expr(Kind::SWITCH_CASE, location)
      , _body(nullptr)
    {}

    /** The set of constant case values to compare to the test expression. */
    const ArrayRef<Expr*>& values() const { return _values; }
    void setValues(const ArrayRef<Expr*>& values) { _values = values; }

    /** The set of constant case ranges to compare to the test expression. */
    const ArrayRef<Expr*>& rangeValues() const { return _rangeValues; }
    void setRangeValues(const ArrayRef<Expr*>& rangeValues) { _rangeValues = rangeValues; }

    /** The body of the case statement. */
    Expr* body() const { return _body; }
    void setBody(Expr* body) { _body = body; }

    /** Dynamic casting support. */
    static bool classof(const CaseStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SWITCH_CASE; }
  private:
    ArrayRef<Expr*> _values;
    ArrayRef<Expr*> _rangeValues;
    Expr* _body;
  };

  /** A switch statement. */
  class SwitchStmt : public Expr {
  public:
    SwitchStmt(Location location)
      : Expr(Kind::SWITCH, location)
      , _testExpr(nullptr)
      , _elseBody(nullptr)
    {}

    /** The test expression. */
    Expr* testExpr() const { return _testExpr; }
    void setTestExpr(Expr* e) { _testExpr = e; }

    /** The list of cases. */
    const ArrayRef<CaseStmt*>& cases() const { return _cases; }
    void setCases(const ArrayRef<CaseStmt*>& cases) { _cases = cases; }

    /** The loop body. */
    Expr* elseBody() const { return _elseBody; }
    void setElseBody(Expr* e) { _elseBody = e; }

    /** Dynamic casting support. */
    static bool classof(const SwitchStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SWITCH; }
  private:
    Expr* _testExpr;
    ArrayRef<CaseStmt*> _cases;
    Expr* _elseBody;
  };

  /** A match pattern. */
  class Pattern : public Expr {
  public:
    Pattern(Location location)
      : Expr(Kind::MATCH_PATTERN, location)
      , _var(nullptr)
      , _body(nullptr)
    {}

    /** The variable that the expression is assigned to. */
    ValueDefn* var() const { return _var; }
    void setVar(ValueDefn* var) { _var = var; }

    /** The body of the case statement. */
    Expr* body() const { return _body; }
    void setBody(Expr* body) { _body = body; }

    /** Dynamic casting support. */
    static bool classof(const Pattern* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::MATCH_PATTERN; }
  private:
    ValueDefn* _var;
    Expr* _body;
  };

  /** A match statement. */
  class MatchStmt : public Expr {
  public:
    MatchStmt(Location location)
      : Expr(Kind::MATCH, location)
      , _testExpr(nullptr)
      , _elseBody(nullptr)
    {}

    /** The test expression. */
    Expr* testExpr() const { return _testExpr; }
    void setTestExpr(Expr* e) { _testExpr = e; }

    /** The list of cases. */
    const ArrayRef<Pattern*>& patterns() const { return _patterns; }
    void setPatterns(const ArrayRef<Pattern*>& patterns) { _patterns = patterns; }

    /** The loop body. */
    Expr* elseBody() const { return _elseBody; }
    void setElseBody(Expr* e) { _elseBody = e; }

    /** Dynamic casting support. */
    static bool classof(const MatchStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::MATCH; }
  private:
    Expr* _testExpr;
  //   Type* _testType;
    ArrayRef<Pattern*> _patterns;
    Expr* _elseBody;
  };

  /** A return statemtn. */
  class ReturnStmt : public Expr {
  public:
    ReturnStmt(Location location, Expr* expr = nullptr)
      : Expr(Kind::RETURN, location)
      , _expr(expr)
    {}
    ReturnStmt(Expr* expr) : Expr(Kind::RETURN, Location()), _expr(expr) {}

    /** The expression to return. */
    Expr* expr() const { return _expr; }

    /** Dynamic casting support. */
    static bool classof(const ReturnStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::RETURN; }
  private:
    Expr* _expr;
  };

  /** A throw statemtn. */
  class ThrowStmt : public Expr {
  public:
    ThrowStmt(Location location, Expr* expr = nullptr)
      : Expr(Kind::THROW, location)
      , _expr(expr)
    {}

    /** The expression to return. */
    Expr* expr() const { return _expr; }

    /** Dynamic casting support. */
    static bool classof(const ThrowStmt* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::THROW; }
  private:
    Expr* _expr;
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

