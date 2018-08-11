#include "tempest/error/diagnostics.hpp"
// #include "tempest/intrinsic/defns.hpp"
// #include "tempest/sema/graph/expr_literal.hpp"
// #include "tempest/sema/graph/expr_op.hpp"
// #include "tempest/sema/graph/expr_stmt.hpp"
// #include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;

  void ResolveTypesPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void ResolveTypesPass::process(Module* mod) {
    _alloc = &mod->semaAlloc();
    // visitList(&scope, mod->members());
  }
}
