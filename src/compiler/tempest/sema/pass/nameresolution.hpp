#ifndef TEMPEST_SEMA_PASS_NAMERESOLUTION_HPP
#define TEMPEST_SEMA_PASS_NAMERESOLUTION_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

namespace tempest::ast {
  class Defn;
  class TypeDefn;
  class FunctionDefn;
  class ValueDefn;
}

namespace tempest::sema::graph {
  class Defn;
}

namespace tempest::sema::names {
  struct LookupScope;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using tempest::sema::names::LookupScope;
  using namespace tempest::sema::graph;

  /** Pass which resolves references to names. */
  class NameResolutionPass {
  public:
    NameResolutionPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

  private:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    llvm::BumpPtrAllocator* _alloc = nullptr;

    bool moreSources() const {
      return _sourcesProcessed < _cu.sourceModules().size();
    }

    bool moreImportSources() const {
      return _importSourcesProcessed < _cu.importSourceModules().size();
    }

    void resolveImports(Module* mod);

    // Defns

    void visitList(LookupScope* scope, DefnArray members);
    void visitTypeDefn(LookupScope* scope, TypeDefn* td);
    void visitFunctionDefn(LookupScope* scope, FunctionDefn* fd);
    void visitValueDefn(LookupScope* scope, ValueDefn* vd);

    // Exprs

    // Types
    Type* resolveType(LookupScope* scope, const ast::Node* ast);
  };
}

#endif
