#ifndef TEMPEST_SEMA_PASS_RESOLVETYPES_HPP
#define TEMPEST_SEMA_PASS_RESOLVETYPES_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

namespace tempest::ast {
  // class Defn;
  // class TypeDefn;
  // class FunctionDefn;
  // class ValueDefn;
}

namespace tempest::sema::graph {
  // class Defn;
  // class Expr;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using namespace tempest::sema::graph;

  /** Pass which resolves references to names. */
  class ResolveTypesPass {
  public:
    ResolveTypesPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

    // Defns

    // void visitList(LookupScope* scope, DefnArray members);
    // void visitTypeDefn(LookupScope* scope, TypeDefn* td);
    // void visitCompositeDefn(LookupScope* scope, TypeDefn* td);
    // void visitEnumDefn(LookupScope* scope, TypeDefn* td);
    // void visitFunctionDefn(LookupScope* scope, FunctionDefn* fd);
    // void visitValueDefn(LookupScope* scope, ValueDefn* vd);

  private:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    llvm::BumpPtrAllocator* _alloc = nullptr;
  };
}

#endif
