#ifndef TEMPEST_COMPILER_COMPILER_HPP
#define TEMPEST_COMPILER_COMPILER_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

namespace tempest::gen {
  class CGModule;
}

namespace tempest::compiler {

  /** Represents a compilation job - all of the source files and libraries to be compiled. */
  class Compiler {
  public:
    Compiler();

    int run();

  private:
    CompilationUnit _cu;

    int addPackageSearchPaths();
    int addSourceFiles();
    void runPasses();
    void outputModule(tempest::gen::CGModule* mod);
  };
}

#endif
