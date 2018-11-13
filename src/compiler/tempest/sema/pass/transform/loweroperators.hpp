#ifndef TEMPEST_SEMA_PASS_TRANSFORM_LOWEROPERATORS_HPP
#define TEMPEST_SEMA_PASS_TRANSFORM_LOWEROPERATORS_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_VISITOR_HPP
  #include "tempest/sema/transform/visitor.hpp"
#endif

namespace tempest::sema::eval {
  struct EvalResult;
}

namespace tempest::sema::graph {
  class BinaryOp;
  class UnaryOp;
}

namespace tempest::sema::names {
  struct LookupScope;
}

namespace tempest::sema::pass::transform {
  using namespace tempest::sema::graph;
  using namespace tempest::compiler;
  using namespace tempest::sema::names;
  using tempest::source::Location;

  class LowerOperatorsTransform : public sema::transform::ExprVisitor {
  public:
    LowerOperatorsTransform(
        CompilationUnit& cu, LookupScope* scope, support::BumpPtrAllocator& alloc)
      : _cu(cu)
      , _scope(scope)
      , _alloc(alloc)
    {}

    Expr* visit(Expr* e);
    Expr* visitInfixOperator(BinaryOp* op);
    Expr* visitUnaryOperator(UnaryOp* op);
    Expr* evalConstOperator(Expr* op);
    Expr* evalResultToExpr(const source::Location& location, eval::EvalResult& result);
    Expr* resolveOperatorName(const Location& loc, const StringRef& name);

  private:
    CompilationUnit& _cu;
    LookupScope* _scope;
    support::BumpPtrAllocator& _alloc;
  };
}

#endif
