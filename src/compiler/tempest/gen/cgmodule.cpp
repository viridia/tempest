#include "tempest/error/diagnostics.h"
#include "tempest/gen/cgmodule.h"
#include "tempest/gen/linkagename.h"

#include <llvm/ADT/Twine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::error::diag;
  using llvm::BasicBlock;
  using llvm::Value;
  using llvm::cast;

  class CGGlobal;
  class CGFunction;

  using tempest::sema::graph::Module;

  CGModule::CGModule(llvm::StringRef name, llvm::LLVMContext& context)
    : _context(context)
    , _irModule(std::make_unique<llvm::Module>(name, context))
  {}

  void CGModule::makeEntryPoint(FunctionDefn* fdef) {
    std::string linkageName;
    getLinkageName(linkageName, fdef);

    llvm::Function * fn = _irModule->getFunction(linkageName);
    if (fn == NULL) {
      diag.error(fdef) << "Function not included in module, cannot be an entry point.";
      return;
    }

    // Generate the main() function type.
    llvm::SmallVector<llvm::Type*, 16> paramTypes;
    // paramTypes.push_back(getMemberType(param, typeArgs));
    auto returnType = llvm::IntegerType::get(_context, 32);
    llvm::Type* mainType = llvm::FunctionType::get(returnType, paramTypes, false);

    // Declare the main() function.
    auto mainFn = llvm::Function::Create(
          cast<llvm::FunctionType>(mainType),
          llvm::Function::ExternalLinkage,
          "main",
          _irModule.get());

    llvm::IRBuilder<> _builder(_context);

    // Call the entry point function.
    BasicBlock * blkEntry = BasicBlock::Create(_context, "entry", mainFn);
    _builder.SetInsertPoint(blkEntry);
    auto result = _builder.CreateCall(fn);
    _builder.CreateRet(result);
  }
}
