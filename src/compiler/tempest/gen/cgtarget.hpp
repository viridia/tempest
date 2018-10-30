#ifndef TEMPEST_GEN_CGTARGET_HPP
#define TEMPEST_GEN_CGTARGET_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#include "llvm/IR/DataLayout.h"

namespace llvm {
  class TargetMachine;
}

namespace tempest::gen {
  class CGModule;

  /** Information about the target machine. */
  class CGTarget {
  public:

    /** Select the LLVM target machine. */
    void select();

    /** Set the target for the specified module. */
    void setModuleTarget(CGModule* mod);

  private:
    std::string _targetTriple;
    llvm::TargetMachine* _targetMachine = nullptr;
  };
}

#endif
