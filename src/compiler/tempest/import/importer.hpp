#ifndef TEMPEST_IMPORT_IMPORTER_H
#define TEMPEST_IMPORT_IMPORTER_H

#ifndef TEMPEST_SEMA_GRAPH_MODULE_H
#include "tempest/sema/graph/module.hpp"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

namespace tempest::import {
  using llvm::StringRef;
  using tempest::sema::graph::Module;
  using tempest::source::ProgramSource;

  /** Represents a location where modules can be found. */
  class Importer {
  public:
    virtual Module* load(StringRef qualifiedName, bool& isPackage) = 0;
    virtual ~Importer() {}
  };
}

#endif
