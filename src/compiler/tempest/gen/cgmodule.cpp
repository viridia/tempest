#include "tempest/error/diagnostics.h"
#include "tempest/gen/cgmodule.h"
#include "tempest/gen/linkagename.h"

#include <llvm/ADT/Twine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

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
    , _diBuilder(*_irModule)
    , _diCompileUnit(nullptr)
    , _diTypeBuilder(_diBuilder, nullptr)
    , _debug(false)
  {
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
      diag.fatal() << error;
      return;
    }
    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, CPU, Features, opt, RM);
    _irModule->setDataLayout(targetMachine->createDataLayout());
    _irModule->setTargetTriple(targetTriple);
  }

  void CGModule::diInit(llvm::StringRef fileName, llvm::StringRef dirName) {
    // Add the current debug info version into the module.
    _irModule->addModuleFlag(
        llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
    _diCompileUnit = _diBuilder.createCompileUnit(
        llvm::dwarf::DW_LANG_C,
        _diBuilder.createFile(fileName, dirName),
        "Tempest Compiler", 0, "", 0);
    _diTypeBuilder.setCompileUnit(_diCompileUnit);
    _debug = true;
  }

  void CGModule::diFinalize() {
    _diBuilder.finalize();
  }

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

    // Create a temporary builder for the entry point func.
    llvm::IRBuilder<> _builder(_context);

    // Call the entry point function.
    BasicBlock * blkEntry = BasicBlock::Create(_context, "entry", mainFn);
    _builder.SetInsertPoint(blkEntry);
    auto result = _builder.CreateCall(fn);
    _builder.CreateRet(result);
  }

  llvm::DIFile* CGModule::getDIFile(ProgramSource* src) {
    if (src) {
      return _diBuilder.createFile(
        llvm::sys::path::filename(src->path()),
        llvm::sys::path::parent_path(src->path()));
    } else {
      return _diCompileUnit->getFile();
    }
  }
}
