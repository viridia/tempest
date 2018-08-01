#include "tempest/error/diagnostics.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/ast/module.hpp"

namespace tempest::compiler {
  using tempest::error::diag;
  using tempest::parse::Parser;

  void CompilationUnit::addSourceFile(llvm::StringRef filepath, llvm::StringRef moduleName) {
    auto mod = _importMgr.getCachedModule(moduleName);
    if (mod) {
      diag.fatal() << "Module already imported";
    } else {
      auto source = std::make_unique<source::FileSource>(filepath, filepath);
      mod = new Module(std::move(source), moduleName);
      _importMgr.addModule(mod);
      _sourceModules.push_back(mod);
      Parser parser(mod->source(), mod->astAlloc());
      auto ast = parser.module();
      if (ast) {
        mod->setAst(ast);
      }
    }
  }
}
