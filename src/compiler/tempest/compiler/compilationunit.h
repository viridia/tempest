#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_H
#define TEMPEST_COMPILER_COMPILATIONUNIT_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPEFACTORY_H
  #include "tempest/sema/graph/typefactory.h"
#endif

namespace tempest::compiler {
  using tempest::sema::graph::TypeStore;

  /** Represents a compilation job - all of the source files and libraries to be compiled. */
  class CompilationUnit {
  public:
    CompilationUnit();

    // addModule
    // addPackage
    // addExternModule

    /** Contains canonicalized instances of derived types. */
    TypeStore& types() { return _types; }

    // specializationStore

  private:
    TypeStore _types;
  };
}

#endif
