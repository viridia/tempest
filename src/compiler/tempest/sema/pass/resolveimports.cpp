#include "tempest/error/diagnostics.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/resolveimports.hpp"
#include "llvm/ADT/SmallString.h"

namespace tempest::sema::pass {
  using tempest::error::diag;
  using llvm::StringRef;
  using tempest::error::diag;

  void ResolveImportsPass::run() {
    // diag.info() << "Resolve imports pass";
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
    // diag.info() << "Resolving imports: " << mod->name();
    auto modAst = static_cast<const ast::Module*>(mod->ast());
    // TODO: Make sure we don't load a given module twice.
    // TODO: Check for circular imports.
    for (auto node : modAst->imports) {
      // At this point I think all we need to do is load the imported modules.
      // We don't need to look up the imported symbols yet.
      auto imp = static_cast<const ast::Import*>(node);
      Module* importMod;
      if (imp->relative == 0) {
         importMod = _cu.importMgr().loadModule(imp->path);
      } else {
        llvm::SmallString<128> impPath;
        llvm::SmallVector<StringRef, 8> pathComponents;
        mod->name().split(pathComponents, '.', false);
        if (size_t(imp->relative) > pathComponents.size()) {
          diag.error(imp) << "Invalid relative path";
          continue;
        }

        // Join the path components from the current module with the relative path.
        for (size_t i = 0; i < pathComponents.size() - imp->relative; i += 1) {
          impPath.append(pathComponents[i]);
          impPath.push_back('.');
        }
        impPath.append(imp->path);
        importMod = _cu.importMgr().loadModule(imp->path);
      }
      if (importMod) {
        if (!importMod->ast()) {
          _cu.importSourceModules().push_back(importMod);
        }
      } else {
        diag.error(imp) << "Imported module not found: " << imp->path;
      }
    }
  }
}
