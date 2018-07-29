#include "tempest/gen/codegen.hpp"
#include "tempest/gen/cgfunctionbuilder.hpp"

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
