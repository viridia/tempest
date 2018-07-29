#ifndef TEMPEST_GEN_INSTR_STMT_HPP
#define TEMPEST_GEN_INSTR_STMT_HPP 1

#ifndef TEMPEST_GEN_INSTR_HPP
  #include "tempest/gen/instr.hpp"
#endif

namespace tempest::gen {
  using tempest::source::Location;
  using llvm::ArrayRef;

  // class ValueDefn;

  /** A statement block. */
  class BlockInstr : public Instr {
  public:
    ArrayRef<Instr*> stmts;
    Instr* result;

    BlockInstr(
        Location location,
        const ArrayRef<Instr*>& stmts,
        Instr* result = nullptr,
        Type* type = nullptr)
      : Instr(Kind::BLOCK, location, type)
      , stmts(stmts)
      , result(result)
    {}

    /** Dynamic casting support. */
    static bool classof(const BlockInstr* e) { return true; }
    static bool classof(const Instr* e) { return e->kind == Kind::BLOCK; }
  };

  /** An if-statement. */
  class IfInstr : public Instr {
  public:
    Instr* test;
    Instr* thenBlock;
    Instr* elseBlock;

    IfInstr(Location location, Instr* test, Instr* thenBlock, Instr* elseBlock, Type* type = nullptr)
      : Instr(Kind::IF, location, type)
      , test(test)
      , thenBlock(thenBlock)
      , elseBlock(elseBlock)
    {}

    /** Dynamic casting support. */
    static bool classof(const IfInstr* e) { return true; }
    static bool classof(const Instr* e) { return e->kind == Kind::IF; }
  };

  /** A while-statement. */
  class WhileInstr : public Instr {
  public:
    Instr* test;
    Instr* body;

    WhileInstr(Location location, Instr* test, Instr* body, Type* type = nullptr)
      : Instr(Kind::WHILE, location, type)
      , test(test)
      , body(body)
    {}

    /** Dynamic casting support. */
    static bool classof(const WhileInstr* e) { return true; }
    static bool classof(const Instr* e) { return e->kind == Kind::WHILE; }
  };

  /** A for-in-statement. */
  // class ForInStmt : public Instr {
  // public:
  //   ForInStmt(
  //       Location location,
  //       Instr* iter = nullptr,
  //       Instr* body = nullptr,
  //       ValueDefn* iterVar = nullptr,
  //       ValueDefn* nextVar = nullptr)
  //     : Instr(Kind::FOR_IN, location)
  //     , _iter(iter)
  //     , _body(body)
  //     , _iterVar(iterVar)
  //     , _nextVar(nullptr)
  //   {}

  //   /** The list of defined variables. */
  //   const ArrayRef<ValueDefn*>& vars() const { return _vars; }
  //   void setVars(const ArrayRef<ValueDefn*>& vars) { _vars = vars; }

  //   /** The init expression. */
  //   Instr* iter() const { return _iter; }
  //   void setIter(Instr* e) { _iter = e; }

  //   /** The loop body. */
  //   Instr* body() const { return _body; }
  //   void setBody(Instr* e) { _body = e; }

  //   /** The variable that contains the iteration expression. */
  //   ValueDefn* iterVar() const { return _iterVar; }
  //   void setIterVar(ValueDefn* vd) { _iterVar = vd; }

  //   /** The variable that contains the value of iterator.next(). This is used to determine
  //       if we've reached the end of the sequence before assigning the values to the individual
  //       loop vars. */
  //   ValueDefn* nextVar() const { return _nextVar; }
  //   void setNextVar(ValueDefn* vd) { _nextVar = vd; }

  //   /** Dynamic casting support. */
  //   static bool classof(const ForInStmt* e) { return true; }
  //   static bool classof(const Instr* e) { return e->kind == Kind::FOR_IN; }
  // private:
  //   ArrayRef<ValueDefn*> _vars;
  //   Instr* _iter;
  //   Instr* _body;
  //   ValueDefn* _iterVar;
  //   ValueDefn* _nextVar;
  // };

  /** A switch case statement. */
  // class CaseStmt : public Instr {
  // public:
  //   CaseStmt(Location location)
  //     : Instr(Kind::SWITCH_CASE, location)
  //     , _body(nullptr)
  //   {}

  //   /** The set of constant case values to compare to the test expression. */
  //   const ArrayRef<Instr*>& values() const { return _values; }
  //   void setValues(const ArrayRef<Instr*>& values) { _values = values; }

  //   /** The set of constant case ranges to compare to the test expression. */
  //   const ArrayRef<Instr*>& rangeValues() const { return _rangeValues; }
  //   void setRangeValues(const ArrayRef<Instr*>& rangeValues) { _rangeValues = rangeValues; }

  //   /** The body of the case statement. */
  //   Instr* body() const { return _body; }
  //   void setBody(Instr* body) { _body = body; }

  //   /** Dynamic casting support. */
  //   static bool classof(const CaseStmt* e) { return true; }
  //   static bool classof(const Instr* e) { return e->kind == Kind::SWITCH_CASE; }
  // private:
  //   ArrayRef<Instr*> _values;
  //   ArrayRef<Instr*> _rangeValues;
  //   Instr* _body;
  // };

  /** A switch statement. */
  // class SwitchStmt : public Instr {
  // public:
  //   SwitchStmt(Location location)
  //     : Instr(Kind::SWITCH, location)
  //     , _testExpr(nullptr)
  //     , _elseBody(nullptr)
  //   {}

  //   /** The test expression. */
  //   Instr* testExpr() const { return _testExpr; }
  //   void setTestExpr(Instr* e) { _testExpr = e; }

  //   /** The list of cases. */
  //   const ArrayRef<CaseStmt*>& cases() const { return _cases; }
  //   void setCases(const ArrayRef<CaseStmt*>& cases) { _cases = cases; }

  //   /** The loop body. */
  //   Instr* elseBody() const { return _elseBody; }
  //   void setElseBody(Instr* e) { _elseBody = e; }

  //   /** Dynamic casting support. */
  //   static bool classof(const SwitchStmt* e) { return true; }
  //   static bool classof(const Instr* e) { return e->kind == Kind::SWITCH; }
  // private:
  //   Instr* _testExpr;
  //   ArrayRef<CaseStmt*> _cases;
  //   Instr* _elseBody;
  // };

  /** A match pattern. */
  // class Pattern : public Instr {
  // public:
  //   Pattern(Location location)
  //     : Instr(Kind::MATCH_PATTERN, location)
  //     , _var(nullptr)
  //     , _body(nullptr)
  //   {}

  //   /** The variable that the expression is assigned to. */
  //   ValueDefn* var() const { return _var; }
  //   void setVar(ValueDefn* var) { _var = var; }

  //   /** The body of the case statement. */
  //   Instr* body() const { return _body; }
  //   void setBody(Instr* body) { _body = body; }

  //   /** Dynamic casting support. */
  //   static bool classof(const Pattern* e) { return true; }
  //   static bool classof(const Instr* e) { return e->kind == Kind::MATCH_PATTERN; }
  // private:
  //   ValueDefn* _var;
  //   Instr* _body;
  // };

  // /** A match statement. */
  // class MatchStmt : public Instr {
  // public:
  //   MatchStmt(Location location)
  //     : Instr(Kind::MATCH, location)
  //     , _testExpr(nullptr)
  //     , _elseBody(nullptr)
  //   {}

  //   /** The test expression. */
  //   Instr* testExpr() const { return _testExpr; }
  //   void setTestExpr(Instr* e) { _testExpr = e; }

  //   /** The list of cases. */
  //   const ArrayRef<Pattern*>& patterns() const { return _patterns; }
  //   void setPatterns(const ArrayRef<Pattern*>& patterns) { _patterns = patterns; }

  //   /** The loop body. */
  //   Instr* elseBody() const { return _elseBody; }
  //   void setElseBody(Instr* e) { _elseBody = e; }

  //   /** Dynamic casting support. */
  //   static bool classof(const MatchStmt* e) { return true; }
  //   static bool classof(const Instr* e) { return e->kind == Kind::MATCH; }
  // private:
  //   Instr* _testExpr;
  // //   Type* _testType;
  //   ArrayRef<Pattern*> _patterns;
  //   Instr* _elseBody;
  // };

  #if 0

  struct Try(Instr) = ExprType.TRY {
    struct Catch {
      location: spark.location.SourceLocation = 1;
      exceptDefn: defn.Var = 2;
      body: Instr = 3;
    }
    body: Instr = 1;
    catchList: list[Catch] = 2;
    else: Instr = 3;
    finally: Instr = 4;
  }

  struct LocalDefn(Instr) = ExprType.LOCAL_DECL {
    defn: defn.Defn = 1;
  }
  struct Intrinsic(Instr) = ExprType.INTRINSIC {}
  #endif

}

#endif
