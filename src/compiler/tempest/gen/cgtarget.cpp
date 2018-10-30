#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/gen/cgtarget.hpp"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/TargetRegistry.h"

namespace tempest::gen {
  using tempest::error::diag;
  using namespace llvm;
  using namespace llvm::sys;

  void CGTarget::select() {
    std::string error;
    _targetTriple = sys::getDefaultTargetTriple();
    auto target = TargetRegistry::lookupTarget(_targetTriple, error);
    if (!target) {
      diag.error() << error;
      return;
    }

    auto cpu = "generic";
    auto features = "";

    TargetOptions opt;
    auto rm = Optional<Reloc::Model>();
    _targetMachine = target->createTargetMachine(_targetTriple, cpu, features, opt, rm);
  }

  void CGTarget::setModuleTarget(CGModule* mod) {
    mod->irModule()->setDataLayout(_targetMachine->createDataLayout());
    mod->irModule()->setTargetTriple(_targetTriple);
  }
}
