#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/gen/linkagename.hpp"
#include "tempest/gen/outputsym.hpp"
#include "tempest/intrinsic/defns.hpp"

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
  using llvm::GlobalVariable;
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
    , _types(context)
    , _diTypeBuilder(_diBuilder, nullptr, _types)
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
    _diTypeBuilder.setDataLayout(&_irModule->getDataLayout());
    _debug = true;
  }

  void CGModule::diFinalize() {
    _diBuilder.finalize();
  }

  void CGModule::makeEntryPoint(FunctionDefn* fdef) {
    std::string linkageName;
    getLinkageName(linkageName, fdef, {});

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
    auto result = _builder.CreateCall(fn, {
      llvm::Constant::getNullValue(llvm::Type::getVoidTy(_context)->getPointerTo()),
    });
    _builder.CreateRet(result);
  }

  llvm::Function* CGModule::getGCAlloc() {
    if (!_gcAlloc) {
      // Signature is gc_alloc(i32, void*) -> void*.
      llvm::Type* paramTypes[2] = {
        llvm::IntegerType::get(_context, 64),
        getClassDescType()->getPointerTo(0),
      };
      llvm::Type* funcType = llvm::FunctionType::get(
        getObjectType()->getPointerTo(1), paramTypes, false);
      _gcAlloc = llvm::Function::Create(
          cast<llvm::FunctionType>(funcType),
          llvm::Function::ExternalLinkage,
          "gc_alloc",
          _irModule.get());
    }

    return _gcAlloc;
  }

  GlobalVariable* CGModule::genClassDescValue(ClassDescriptorSym* sym) {
    if (sym->desc) {
      return sym->desc;
    }
    std::string linkageName;
    linkageName.reserve(64);
    getLinkageName(linkageName, sym->typeDefn, sym->typeArgs);
    linkageName.append("::cldesc");
    sym->desc = new llvm::GlobalVariable(
        *_irModule, getClassDescType(), true,
        llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, linkageName);
    return sym->desc;
  }

  GlobalVariable* CGModule::genClassDesc(ClassDescriptorSym* sym) {
    auto clsDesc = genClassDescValue(sym);
    auto clsDescType = getClassDescType();
    llvm::Constant* clsDescProps[3] = {
      llvm::ConstantPointerNull::get(clsDescType->getPointerTo()),
      llvm::ConstantPointerNull::get(
          llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo()),
      llvm::ConstantPointerNull::get(
          llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo()),
    };
    clsDesc->setInitializer(llvm::ConstantStruct::get(clsDescType, clsDescProps));
    return clsDesc;
  }

  llvm::Type* CGModule::getObjectType() {
    if (!_objectType) {
      _objectType = _types.get(intrinsic::IntrinsicDefns::get()->objectClass->type(), {});
    }
    return _objectType;
  }

  llvm::StructType* CGModule::getClassDescType() {
    if (!_classDescType) {
      // Class descriptor fields:
      // - base class
      // - interface table
      // - method table
      auto cdType = llvm::StructType::create(_context, "ClassDescriptor");
      llvm::Type* descFieldTypes[3] = {
        cdType->getPointerTo(),
        llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo(),
        llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo(),
      };
      cdType->setBody(descFieldTypes);
      _classDescType = cdType;
    }
    return _classDescType;
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
