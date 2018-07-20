#include "tempest/gen/cgmodule.h"
#include <llvm/ADT/Twine.h>

namespace tempest::gen {
  class CGGlobal;
  class CGFunction;

  using tempest::sema::graph::Module;

  CGModule::CGModule(llvm::StringRef name, llvm::LLVMContext& context)
    : _irModule(std::make_unique<llvm::Module>(name, context))
    , _types(context, _alloc)
  {
  }
}
