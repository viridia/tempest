#include "tempest/opt/basicopts.h"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <memory>

namespace tempest::opt {
  using namespace llvm;
  BasicOpts::BasicOpts(CGModule* mod) {
    _fpm = std::make_unique<llvm::legacy::FunctionPassManager>(mod->irModule());
    _fpm->add(createInstructionCombiningPass());
    _fpm->add(createReassociatePass());
    _fpm->add(createGVNPass());
    _fpm->add(createCFGSimplificationPass());
    _fpm->doInitialization();
  }

  void BasicOpts::run(llvm::Function* fn) {
    _fpm->run(*fn);
  }
}
