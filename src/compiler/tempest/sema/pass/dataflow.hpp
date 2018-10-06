#ifndef TEMPEST_SEMA_PASS_DATAFLOW_HPP
#define TEMPEST_SEMA_PASS_DATAFLOW_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#include "llvm/ADT/BitVector.h"

namespace tempest::ast {
  class Defn;
  class TypeDefn;
  class FunctionDefn;
  class Oper;
  class ValueDefn;
}

namespace tempest::sema::graph {
  class Defn;
  class Expr;
}

namespace tempest::sema::names {
  struct LookupScope;
}

namespace tempest::sema::pass {
  using namespace tempest::sema::graph;
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using tempest::sema::names::LookupScope;
  using llvm::BitVector;

  struct FlowState {
    BitVector localVarsSet;
    BitVector localVarsRead;
    BitVector memberVarsSet;

    FlowState() {}
    FlowState(size_t numLocalVars, size_t numMemberVars) {
      localVarsSet.resize(numLocalVars);
      localVarsRead.resize(numLocalVars);
      memberVarsSet.resize(numMemberVars);
    }

    size_t numLocalVars() const { return localVarsSet.size(); }
    size_t numMemberVars() const { return memberVarsSet.size(); }
  };

  /** Pass which resolves references to names. */
  class DataFlowPass {
  public:
    DataFlowPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

    // Defns

    void visitList(DefnArray members);
    void visitTypeDefn(TypeDefn* td);
    void visitCompositeDefn(TypeDefn* td);
    void visitEnumDefn(TypeDefn* td);
    void visitFunctionDefn(FunctionDefn* fd);
    void visitValueDefn(ValueDefn* vd);

    // Exprs

    void visitExpr(Expr* e, FlowState& flow);

  private:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    FunctionDefn* _currentMethod = nullptr;
    tempest::support::BumpPtrAllocator* _alloc = nullptr;
  };
}

#endif
