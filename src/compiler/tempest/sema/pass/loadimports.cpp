#include "tempest/error/diagnostics.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/loadimports.hpp"

namespace tempest::sema::pass {
  using tempest::error::diag;
  using llvm::StringRef;
  using tempest::error::diag;

  void LoadImportsPass::run() {
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

  void LoadImportsPass::process(Module* mod) {
    // diag.info() << "Resolving imports: " << mod->name();
    auto modAst = static_cast<const ast::Module*>(mod->ast());
    // TODO: Make sure we don't load a given module twice from the same module.
    // TODO: Check for circular imports.
    if (!modAst) {
      return;
    }
    for (auto node : modAst->imports) {
      // At this point I think all we need to do is load the imported modules.
      // We don't need to look up the imported symbols yet.
      auto imp = static_cast<const ast::Import*>(node);
      Module* importMod;
      if (imp->relative == 0) {
        importMod = _cu.importMgr().loadModule(imp->path);
      } else {
        importMod = _cu.importMgr().loadModuleRelative(
            imp->location, mod->name(), imp->relative, imp->path);
      }
      if (importMod) {
        if (importMod->group() == sema::graph::ModuleGroup::IMPORT_SOURCE) {
          auto it = std::find(
              _cu.importSourceModules().begin(),
              _cu.importSourceModules().end(),
              importMod);
          if (it == _cu.importSourceModules().end()) {
            _cu.importSourceModules().push_back(importMod);
          }
        }
      } else {
        diag.error(imp) << "Imported module not found: " << imp->path;
      }
    }
  }
}
