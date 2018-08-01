#ifndef TEMPEST_SEMA_PASS_RESOLVEIMPORTS_HPP
#define TEMPEST_SEMA_PASS_RESOLVEIMPORTS_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;

  /** Pass which constructs the semantic graph, and loads in additional import modules
      if they are reachable. */
  class ResolveImportsPass {
  public:
    ResolveImportsPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

  private:
    bool moreSources() const {
      return _sourcesProcessed < _cu.sourceModules().size();
    }

    bool moreImportSources() const {
      return _importSourcesProcessed < _cu.importSourceModules().size();
    }

    void process(Module* mod);

    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
  };

}

#endif
