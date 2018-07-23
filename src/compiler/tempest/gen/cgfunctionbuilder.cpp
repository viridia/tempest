#include "tempest/error/diagnostics.h"
#include "tempest/gen/codegen.h"
#include "tempest/gen/cgfunctionbuilder.h"
#include "tempest/gen/linkagename.h"
#include "tempest/sema/graph/expr_literal.h"
#include "tempest/sema/graph/expr_stmt.h"
#include "tempest/sema/graph/primitivetype.h"

#include <llvm/IR/Constants.h>
#include <llvm/Support/Casting.h>

namespace tempest::gen {
  using tempest::error::diag;
  using namespace tempest::sema::graph;
  using llvm::cast;
  using llvm::BasicBlock;

  CGFunctionBuilder::CGFunctionBuilder(CodeGen& gen, CGModule* module)
    : _gen(gen)
    , _module(module)
    , _builder(gen.context)
    , _irModule(module->irModule())
  {
    (void)_module;
  }

  llvm::Function* CGFunctionBuilder::genFunctionValue(FunctionDefn* func) {
    std::string linkageName;
    getLinkageName(linkageName, func);

    // DASSERT_OBJ(fdef->passes().isFinished(FunctionDefn::ParameterTypePass), fdef);
    llvm::Function * fn = _irModule->getFunction(linkageName);
    if (fn != NULL) {
      return fn;
    }

    // DASSERT_OBJ(!fdef->isAbstract(), fdef);
    // DASSERT_OBJ(!fdef->isInterfaceMethod(), fdef);
    // DASSERT_OBJ(!fdef->isUndefined(), fdef);
    // DASSERT_OBJ(!fdef->isIntrinsic(), fdef);
    // DASSERT_OBJ(fdef->isSingular(), fdef);
    // DASSERT_OBJ(fdef->passes().isFinished(FunctionDefn::ParameterTypePass), fdef);
    // DASSERT_OBJ(fdef->passes().isFinished(FunctionDefn::ReturnTypePass), fdef);
    // DASSERT_OBJ(fdef->defnType() != Defn::Macro, fdef);

    llvm::Type* funcType = _gen.types().get(func->type());

    // // If it's a function from a different module...
    // if (fdef->module() != module_) {
    //   fn = Function::Create(
    //       cast<llvm::FunctionType>(funcType->irType()),
    //       Function::ExternalLinkage, fdef->linkageName(),
    //       irModule_);
    //   return fn;
    // }

    // // Generate the function reference
    // DASSERT_OBJ(funcType->isSingular(), fdef);

    fn = llvm::Function::Create(
        cast<llvm::FunctionType>(funcType),
        llvm::Function::ExternalLinkage,
        linkageName,
        _irModule);

    // if (fdef->flags() & FunctionDefn::NoInline) {
    //   fn->addFnAttr(llvm::Attribute::NoInline);
    // }

    return fn;
  }

  llvm::Function* CGFunctionBuilder::genFunction(FunctionDefn* func) {
    _irFunction = genFunctionValue(func);

    // Don't generate undefined functions.
    // if (fdef->isUndefined() || fdef->isAbstract() || fdef->isInterfaceMethod()) {
    //   return true;
    // }

    // DASSERT_OBJ(fdef->isSingular(), fdef);
    // DASSERT_OBJ(fdef->type(), fdef);
    // DASSERT_OBJ(fdef->type()->isSingular(), fdef);

    // // Don't generate intrinsic functions.
    // if (fdef->isIntrinsic()) {
    //   return true;
    // }

    // Don't generate a function if it has been merged to another function
    // if (fdef->mergeTo() != NULL || fdef->isUndefined()) {
    //   return true;
    // }

    // Create the function
    // Function * f = genFunctionValue(fdef);

    if (func->body() && _irFunction->getBasicBlockList().empty()) {
      // FunctionType * ftype = fdef->type();

      // if (fdef->isSynthetic()) {
      //   f->setLinkage(GlobalValue::LinkOnceODRLinkage);
      // }

      // if (gcEnabled_) {
      //   if (SsGC) {
      //     f->setGC("shadow-stack");
      //   } else {
      //     f->setGC("tart-gc");
      //   }
      // }

      // if (debug_) {
      //   dbgContext_ = genDISubprogram(fdef);
      //   //dbgContext_ = genLexicalBlock(fdef->location());
      //   dbgInlineContext_ = DIScope();
      //   setDebugLocation(fdef->location());
      // }

      // BasicBlock * prologue = BasicBlock::Create(context_, "prologue", f);

      // Create the LLVM Basic Blocks corresponding to each high level BB.
  //    BlockList & blocks = fdef->blocks();
  //    for (BlockList::iterator b = blocks.begin(); b != blocks.end(); ++b) {
  //      Block * blk = *b;
  //      blk->setIRBlock(BasicBlock::Create(context_, blk->label(), f));
  //    }

      // builder_.SetInsertPoint(prologue);

      // Handle the explicit parameters
      // unsigned param_index = 0;
      // Function::arg_iterator it = f->arg_begin();

      // Value * saveStructRet = structRet_;
      // if (ftype->isStructReturn()) {
      //   it->addAttr(llvm::Attribute::StructRet);
      //   structRet_ = it;
      //   ++it;
      // }

      // // Handle the 'self' parameter
      // if (ftype->selfParam() != NULL) {
      //   ParameterDefn * selfParam = ftype->selfParam();
      //   const Type * selfParamType = selfParam->type().unqualified();
      //   DASSERT_OBJ(fdef->storageClass() == Storage_Instance ||
      //       fdef->storageClass() == Storage_Local, fdef);
      //   DASSERT_OBJ(it != f->arg_end(), ftype);

      //   // Check if the self param is a root.
      //   if (selfParamType->isReferenceType()) {
      //     selfParam->setFlag(ParameterDefn::LValueParam, true);
      //     Value * selfAlloca = builder_.CreateAlloca(
      //         selfParam->type()->irEmbeddedType(), 0, "self.alloca");
      //     builder_.CreateStore(it, selfAlloca);
      //     selfParam->setIRValue(selfAlloca);
      //     markGCRoot(selfAlloca, NULL, "self.alloca");
      //   } else {
      //     // Since selfParam is always a pointer, we don't need to mark the object pointed
      //     // to as a root, since the next call frame up is responsible for tracing it.
      //     ftype->selfParam()->setIRValue(it);
      //   }

      //   it->setName("self");
      //   ++it;
      // }

      // If this function needs to make allocations, cache a copy of the
      // allocation context pointer for this thread, since it can on some
      // platforms be expensive to look up.
      // if (fdef->flags() & FunctionDefn::MakesAllocs) {
      //   Function * gcGetAllocContext = genFunctionValue(gc_allocContext);
      //   gcAllocContext_ = builder_.CreateCall(gcGetAllocContext, "allocCtx");
      // }

  //     for (; it != f->arg_end(); ++it, ++param_index) {

  //       // Set the name of the Nth parameter
  //       ParameterDefn * param = ftype->params()[param_index];
  //       DASSERT_OBJ(param != NULL, fdef);
  //       DASSERT_OBJ(param->storageClass() == Storage_Local, param);
  //       QualifiedType paramType = param->internalType();
  //       it->setName(param->name());
  //       Value * paramValue = it;

  //       // If the parameter is a shared reference, then create the shared ref.
  //       if (param->isSharedRef()) {
  //         genLocalVar(param, paramValue);
  //         genGCRoot(param->irValue(), param->sharedRefType(), param->name());
  //         continue;
  //       }

  //       // If the parameter type contains any reference types, then the parameter needs
  //       // to be a root.
  //       bool paramIsRoot = false;
  //       if (paramType->isReferenceType()) {
  //         param->setFlag(ParameterDefn::LValueParam, true);
  //         paramIsRoot = true;
  //       } else if (paramType->containsReferenceType()) {
  //         // TODO: Handle roots of various shapes
  //         //param->setFlag(ParameterDefn::LValueParam, true);
  //       }

  //       // See if we need to make a local copy of the param.
  //       if (param->isLValue()) {
  //         Value * paramAlloca = builder_.CreateAlloca(paramType->irEmbeddedType(), 0, param->name());
  //         param->setIRValue(paramAlloca);

  //         if (paramType->typeShape() == Shape_Large_Value) {
  //           paramValue = builder_.CreateLoad(paramValue);
  //         }

  //         builder_.CreateStore(paramValue, paramAlloca);
  //         if (paramIsRoot) {
  //           genGCRoot(paramAlloca, paramType.unqualified(), param->name());
  //         }
  //       } else {
  //         param->setIRValue(paramValue);
  //       }
  //     }

  //     // Generate the body
  //     Function * saveFn = currentFn_;
  //     currentFn_ = f;
  // #if 0
  //     if (fdef->isGenerator()) {
  //       assert(false);
  //     } else {
  // #endif
  //       genLocalStorage(fdef->localScopes());
  //       genDISubprogramStart(fdef);
  //       genLocalRoots(fdef->localScopes());

        BasicBlock * blkEntry = BasicBlock::Create(_gen.context, "entry", _irFunction);
        _builder.SetInsertPoint(blkEntry);
        visitExpr(func->body());

        if (!atTerminator()) {
          // if (func->type()->returnType->isVoidType()) {
          //   _builder.CreateRetVoid();
          // } else {
            // TODO: Use the location from the last statement of the function.
            diag.error(func) << "Missing return statement at end of non-void function.";
          // }
        }

        // gcAllocContext_ = NULL;
  #if 0
      }
  #endif

      // builder_.SetInsertPoint(prologue);
      // builder_.CreateBr(blkEntry);

      // currentFn_ = saveFn;
      // structRet_ = saveStructRet;

      // if (!diag.inRecovery()) {
      //   if (verifyFunction(*f, PrintMessageAction)) {
      //     f->dump();
      //     DFAIL("Function failed to verify");
      //   }
      // }

      //if (debug_ && !dbgContext_.isNull() && !dbgContext_.Verify()) {
      //  dbgContext_.Verify();
      //  DFAIL("BAD DBG");
      //}

      // dbgContext_ = DIScope();
      // dbgInlineContext_ = DIScope();
      // builder_.ClearInsertionPoint();
      // builder_.SetCurrentDebugLocation(llvm::DebugLoc());
    }

    // return true;

    return _irFunction;
  }

  // llvm::Function* CGFunctionBuilder::build(FunctionDefn* fd) {
  //   // Constant* c = mod->getOrInsertFunction(
  //   //     "gcd",
  //   //     IntegerType::get(32),
  //   //     IntegerType::get(32),
  //   //     IntegerType::get(32),
  //   //     NULL);
  //   // Function* gcd = cast<Function>(c);    

  //   visitExpr(fd->body());
  //   return nullptr;
  // }

  llvm::Value* CGFunctionBuilder::visitExpr(Expr* expr) {
    switch (expr->kind) {
      case Expr::Kind::BLOCK:
        return visitBlockStmt(static_cast<BlockStmt*>(expr));
      case Expr::Kind::RETURN:
        return visitReturnStmt(static_cast<ReturnStmt*>(expr));
      case Expr::Kind::INTEGER_LITERAL:
        return visitIntegerLiteral(static_cast<IntegerLiteral*>(expr));
      default:
        assert(false && "Invalid expression type");
    }
  }

  llvm::Value* CGFunctionBuilder::visitBlockStmt(BlockStmt* blk) {
    llvm::Value * result = nullptr;
    // size_t savedRootCount = rootStackSize();
    // pushRoots(in->scope());

    // DIScope saveContext = dbgContext_;
    // if (in->scope() != NULL && debug_) {
    //   dbgContext_ = genLexicalBlock(in->location());
    // }

    for (auto it = blk->stmts().begin(); it != blk->stmts().end(); ++it) {
      if (atTerminator()) {
        diag.warn(*it) << "Unreachable statement";
      }
      // setDebugLocation((*it)->location());
      result = visitExpr(*it);
      if (result == NULL) {
        //endLexicalBlock(savedScope);
        return NULL;
      }
    }

    // if (!atTerminator()) {
    //   popRootStack(savedRootCount);
    // }

    // dbgContext_ = saveContext;
    return result ? result : voidValue();
  }

  llvm::Value* CGFunctionBuilder::visitReturnStmt(ReturnStmt* in) {
    // Execute all cleanups
    // for (BlockExits * be = blockExits_; be != NULL; be = be->parent()) {
    //   if (be->cleanupBlock() != NULL) {
    //     callCleanup(be);
    //   }
    // }

    // setDebugLocation(in->location());
    if (in->expr() == NULL) {
      // Return nothing
      _builder.CreateRet(NULL);
    } else {
      // Generate the return value.
      Expr* returnVal = in->expr();
      llvm::Value* value = visitExpr(returnVal);
      if (value == NULL) {
        return NULL;
      }

      // Handle struct returns and other conventions.
      // QualifiedType returnType = returnVal->canonicalType();
      // TypeShape returnShape = returnType->typeShape();
      // DASSERT(value != NULL);

      // if (returnShape == Shape_Large_Value) {
      //   value = loadValue(value, returnVal);
      // }

      // if (returnType->irEmbeddedType() != value->getType()) {
      //   returnType->irEmbeddedType()->dump();
      //   value->getType()->dump();
      //   DASSERT(false);
      // }
      // DASSERT_TYPE_EQ(returnVal, returnType->irEmbeddedType(), value->getType());

      // if (structRet_ != NULL) {
      //   _builder.CreateStore(value, structRet_);
      //   _builder.CreateRet(NULL);
      // } else {
      _builder.CreateRet(value);
      // }
    }

    return voidValue();
  }

  llvm::Value* CGFunctionBuilder::visitIntegerLiteral(IntegerLiteral* value) {
    return llvm::Constant::getIntegerValue(types().get(value->type()), value->value());
  }

  CGTypeBuilder& CGFunctionBuilder::types() { return _gen.types(); }

  llvm::Value* CGFunctionBuilder::voidValue() const {
    return llvm::UndefValue::get(llvm::Type::getVoidTy(_gen.context));
  }

  bool CGFunctionBuilder::atTerminator() const {
    return _builder.GetInsertBlock() == NULL || _builder.GetInsertBlock()->getTerminator() != NULL;
  }
}
