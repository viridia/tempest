#include "tempest/gen/codegen.hpp"
#include "tempest/gen/cgfunctionbuilder.hpp"
#include "tempest/gen/symbolstore.hpp"

namespace tempest::gen {
  CGModule* CodeGen::createModule(llvm::StringRef name) {
    _module = new CGModule(name, context);
    return _module;
  }

  void CodeGen::genSymbols(SymbolStore& symbols) {
    for (auto sym : symbols.list()) {
      if (auto fsym = dyn_cast<FunctionSym>(sym)) {
        CGFunctionBuilder builder(*this, _module, fsym->typeArgs);
        assert(fsym->body);
        fsym->fnVal = builder.genFunctionValue(fsym->function);
      } else if (auto clsSym = dyn_cast<ClassDescriptorSym>(sym)) {
        _module->genClassDesc(clsSym);
      }
    }

    for (auto sym : symbols.list()) {
      if (auto fsym = dyn_cast<FunctionSym>(sym)) {
        CGFunctionBuilder builder(*this, _module, fsym->typeArgs);
        assert(fsym->body);
        builder.genFunction(fsym->function, fsym->body);
      }
    }
  }

  llvm::Function* CodeGen::genFunctionValue(FunctionDefn* func, ArrayRef<const Type*> typeArgs) {
    CGFunctionBuilder builder(*this, _module, typeArgs);
    return builder.genFunctionValue(func);
  }

  llvm::Function* CodeGen::genFunction(FunctionDefn* func, ArrayRef<const Type*> typeArgs) {
    CGFunctionBuilder builder(*this, _module, typeArgs);
    return builder.genFunction(func, func->body());
  }
}
