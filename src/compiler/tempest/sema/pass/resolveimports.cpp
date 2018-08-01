#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/resolveimports.hpp"

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;

  void ResolveImportsPass::run() {
    diag.info() << "Resolve imports pass";
    while (moreSources() || moreImportSources()) {
      while (moreSources()) {
        process(_cu.sourceModules()[_sourcesProcessed++]);
      }
      while (moreImportSources()) {
        process(_cu.importSourceModules()[_importSourcesProcessed++]);
      }
    }
  }

  void ResolveImportsPass::process(Module* mod) {
    diag.info() << "Resolving imports: " << mod->name();
  }
}

