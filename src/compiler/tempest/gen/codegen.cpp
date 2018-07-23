#include "tempest/gen/codegen.h"
#include "tempest/gen/cgfunctionbuilder.h"

namespace tempest::gen {
  CGModule* CodeGen::createModule(llvm::StringRef name) {
    _module = new CGModule(name, context);
    return _module;
  }

  llvm::Function* CodeGen::genFunctionValue(FunctionDefn* func) {
    CGFunctionBuilder builder(*this, _module);
    return builder.genFunctionValue(func);
  }

  llvm::Function* CodeGen::genFunction(FunctionDefn* func) {
    CGFunctionBuilder builder(*this, _module);
    return builder.genFunction(func);
  }
}
