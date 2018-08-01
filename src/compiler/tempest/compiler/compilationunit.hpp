#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
#define TEMPEST_COMPILER_COMPILATIONUNIT_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#ifndef TEMPEST_IMPORT_IMPORTMGR_HPP
  #include "tempest/import/importmgr.hpp"
#endif

#include <vector>

namespace tempest::compiler {
  using tempest::sema::graph::Module;
  using tempest::sema::graph::TypeStore;
  using tempest::import::ImportMgr;

  /** Represents a compilation job - all of the source files and libraries to be compiled. */
  class CompilationUnit {
  public:
    // CompilationUnit();

    /** Add a source file to be compiled. */
    void addSourceFile(llvm::StringRef filepath, llvm::StringRef moduleName);

    /** The initial set of modules to be compiled. These are the modules that were explicitly
        requested by the user. */
    std::vector<Module*> &sourceModules() { return _sourceModules; }

    /** Additional source moduls that need to be compiled because they were
        import-reachable from the set of source modules. */
    std::vector<Module*> &importSourceModules() { return _importSourceModules; }

    // addModule
    // addExternModule

    /** Contains canonicalized instances of derived types. */
    TypeStore& types() { return _types; }

    /** Manages imports of modules. */
    ImportMgr& importMgr() { return _importMgr; }

    // specializationStore

  private:
    TypeStore _types;
    ImportMgr _importMgr;
    std::vector<Module*> _sourceModules;
    std::vector<Module*> _importSourceModules;
  };
}

#endif
