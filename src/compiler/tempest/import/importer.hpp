#ifndef TEMPEST_IMPORT_IMPORTER_H
#define TEMPEST_IMPORT_IMPORTER_H

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_H
#include "tempest/sema/graph/module.hpp"
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
