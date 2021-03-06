#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
#define TEMPEST_COMPILER_COMPILATIONUNIT_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SPECSTORE_HPP
  #include "tempest/sema/graph/specstore.hpp"
#endif

#ifndef TEMPEST_IMPORT_IMPORTMGR_HPP
  #include "tempest/import/importmgr.hpp"
#endif

#ifndef TEMPEST_GEN_SYMBOLSTORE_HPP
  #include "tempest/gen/symbolstore.hpp"
#endif

#include <vector>

namespace tempest::compiler {
  using tempest::sema::graph::Module;
  using tempest::sema::graph::SpecializationStore;
  using tempest::sema::graph::TypeStore;
  using tempest::import::ImportMgr;
  using tempest::gen::SymbolStore;

  /** Represents a compilation job - all of the source files and libraries to be compiled. */
  class CompilationUnit {
  public:
    CompilationUnit() : _spec(_types.alloc()) {}

    /** Add a source file to be compiled. */
    void addSourceFile(llvm::StringRef filepath, llvm::StringRef moduleName);

    /** The initial set of modules to be compiled. These are the modules that were explicitly
        requested by the user. */
    std::vector<Module*> &sourceModules() { return _sourceModules; }

    /** Additional source moduls that need to be compiled because they were
        import-reachable from the set of source modules. */
    std::vector<Module*> &importSourceModules() { return _importSourceModules; }

    /** File to write output bitcode. */
    SmallString<32>& outputFile() { return _outputFile; }

    /** File to write output bitcode. */
    SmallString<32>& outputModName() { return _outputModName; }

    // addModule
    // addExternModule

    /** Contains canonicalized instances of derived types. */
    TypeStore& types() { return _types; }

    /** Contains canonicalized instances of derived types. */
    SpecializationStore& spec() { return _spec; }

    /** Repository of output symbols to be emitted. */
    SymbolStore& symbols() { return _symbols; }

    /** Manages imports of modules. */
    ImportMgr& importMgr() { return _importMgr; }

    // Static instance of current compilation unit.
    static CompilationUnit* theCU;

  private:
    TypeStore _types;
    SpecializationStore _spec;
    SymbolStore _symbols;
    ImportMgr _importMgr;
    std::vector<Module*> _sourceModules;
    std::vector<Module*> _importSourceModules;
    SmallString<32> _outputFile;
    SmallString<32> _outputModName;
  };
}

#endif
