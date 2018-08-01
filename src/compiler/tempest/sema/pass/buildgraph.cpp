#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/buildgraph.hpp"

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;

  void BuildGraphPass::run() {
    diag.info() << "Build graph pass";
    while (moreSources() || moreImportSources()) {
      while (moreSources()) {
        _sourcesProcessed += 1;
      }
      while (moreImportSources()) {
        _importSourcesProcessed += 1;
      }
    }
  }
}

