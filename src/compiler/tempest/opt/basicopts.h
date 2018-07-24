#ifndef TEMPEST_OPT_BASICOPTS_H
#define TEMPEST_OPT_BASICOPTS_H 1

#ifndef TEMPEST_GEN_CGMODULE_H
  #include "tempest/gen/cgmodule.h"
#endif

#ifndef LLVM_IR_LEGACYPASSMANAGER_H
  #include <llvm/IR/LegacyPassManager.h>
#endif

#include <memory>

namespace tempest::opt {
  using namespace tempest::gen;

  /** Basic LLVM module optimization passes. */
  class BasicOpts {
  public:
    BasicOpts(CGModule* mod);

    void run(llvm::Function* fn);

  private:
    std::unique_ptr<llvm::legacy::FunctionPassManager> _fpm;
  };
}

#endif
