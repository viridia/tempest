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

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::error::diag;
  using llvm::BasicBlock;
  using llvm::GlobalVariable;
  using llvm::Value;

  class CGGlobal;
  class CGFunction;

  using tempest::sema::graph::Module;

  CGModule::CGModule(llvm::StringRef name, llvm::LLVMContext& context)
    : _context(context)
    , _irModule(std::make_unique<llvm::Module>(name, context))
    , _diBuilder(*_irModule)
    , _diCompileUnit(nullptr)
    , _types(context, &_irModule->getDataLayout())
    , _diTypeBuilder(_diBuilder, nullptr, _types)
    , _debug(false)
  {
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

  Value* CGModule::genVarValue(GlobalVarSym* vsym) {
    // Don't generate the IR if we've already done so.
    if (vsym->global != NULL) {
      return vsym->global;
    }

    return genGlobalVar(vsym);
  }

  llvm::Constant* CGModule::genGlobalVar(GlobalVarSym* vsym) {
    assert(vsym->kind == OutputSym::Kind::GLOBAL);
    assert(vsym->global == nullptr);
    assert(vsym->varDefn->isStatic() || vsym->varDefn->isGlobal());

    std::string linkageName;
    linkageName.reserve(64);
    getLinkageName(linkageName, vsym->varDefn, vsym->typeArgs);

    GlobalVariable* gv = _irModule->getGlobalVariable(linkageName);
    if (gv != nullptr) {
      return gv;
    }

    auto var = vsym->varDefn;
    // const Type* varType = var->type().unqualified();
    assert(var->type() != nullptr);

    // Create the global variable
    auto linkType = llvm::Function::ExternalLinkage;
    // if (var->isSynthetic()) {
    //   linkType = Function::LinkOnceAnyLinkage;
    // }

    // The reason that this is irType instead of irEmbeddedType is because LLVM always turns
    // the type of a global variable into a pointer anyway.
    llvm::Type* irType = _types.get(var->type(), vsym->typeArgs);
    gv = new GlobalVariable(*_irModule, irType, false, linkType, nullptr, linkageName,
        NULL /*, var->isThreadLocal() */);

    // Only supply an initialization expression if the variable was
    // defined in this module - otherwise, it's an external declaration.
    // if (var->module() == module_ || var->isSynthetic()) {
    //   // addStaticRoot(gv, varType);
    //   // if (debug_) {
    //   //   genDIGlobalVariable(var, gv);
    //   // }

    //   // If it has an initialization expression
    //   const Expr* initExpr = var->init();
    //   if (initExpr != NULL) {
    //     if (initExpr->isConstant()) {
    //       Constant * initValue = genConstExpr(initExpr);
    //       if (initValue == NULL) {
    //         return NULL;
    //       }

    //       if (varType->isReferenceType()) {
    //         initValue = new GlobalVariable(
    //             *irModule_, initValue->getType(), false, linkType, initValue,
    //             var->linkageName() + ".init");
    //         initValue = llvm::ConstantExpr::getPointerCast(initValue, varType->irEmbeddedType());
    //       }

    //       gv->setInitializer(initValue);
    //     } else {
    //       genModuleInitFunc();

    //       // Add it to the module init function
    //       BasicBlock * savePoint = builder_.GetInsertBlock();
    //       builder_.SetInsertPoint(moduleInitBlock_);

    //       // Generate the expression.
    //       Value * initValue = genExpr(initExpr);
    //       if (initValue != NULL) {
    //         gv->setInitializer(llvm::Constant::getNullValue(irType));
    //         builder_.CreateStore(initValue, gv);
    //       }

    //       if (savePoint != NULL) {
    //         builder_.SetInsertPoint(savePoint);
    //       }
    //     }
    //   } else if (!var->isExtern()) {
    //     // No initializer, so set the value to zerofill.
    //     gv->setInitializer(llvm::Constant::getNullValue(irType));
    //   }
    // }

    return gv;
  }

  llvm::Function* CGModule::getGCAlloc() {
    if (!_gcAlloc) {
      // Signature is gc_alloc(i32, void*) -> void*.
      llvm::Type* paramTypes[2] = {
        llvm::IntegerType::get(_context, 64),
        _types.getClassDescType()->getPointerTo(0),
      };
      llvm::Type* funcType = llvm::FunctionType::get(
        _types.getObjectType()->getPointerTo(1), paramTypes, false);
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
        *_irModule, _types.getClassDescType(), true,
        llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, linkageName);
    return sym->desc;
  }

  GlobalVariable* CGModule::genClassDesc(ClassDescriptorSym* sym) {
    auto clsDesc = genClassDescValue(sym);
    auto clsDescType = _types.getClassDescType();
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
