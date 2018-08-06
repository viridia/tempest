#ifndef TEMPEST_SEMA_PASS_BUILDGRAPH_HPP
#define TEMPEST_SEMA_PASS_BUILDGRAPH_HPP 1

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
}

namespace tempest::sema::graph {
  class Defn;
  class GenericDefn;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using namespace tempest::sema::graph;

  /** Pass which constructs the semantic graph, and loads in additional import modules
      if they are reachable. */
  class BuildGraphPass {
  public:
    BuildGraphPass(CompilationUnit& cu) : _cu(cu) {}

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

    void createMembers(
        const ast::NodeList& memberAsts,
        Member* parent,
        DefnList& memberList,
        SymbolTable* memberScope);
    Defn* createDefn(const ast::Node* node, Member* parent);
    void createParamList(
        const ast::NodeList& paramAsts,
        Member* parent,
        std::vector<ParameterDefn*>& paramList,
        SymbolTable* paramScope);
    void createTypeParamList(
        const ast::NodeList& paramAsts,
        GenericDefn* parent,
        SymbolTable* paramScope);
    Visibility astVisibility(const ast::Defn* d);
  };
}

#endif
