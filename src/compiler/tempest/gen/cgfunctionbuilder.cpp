#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/codegen.hpp"
#include "tempest/gen/cgfunctionbuilder.hpp"
#include "tempest/gen/linkagename.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/Support/Casting.h>

namespace tempest::gen {
  using tempest::error::diag;
  using namespace tempest::sema::graph;
  using llvm::cast;
  using llvm::dyn_cast;
  using llvm::BasicBlock;
  using llvm::DIFile;
  using llvm::DINode;
  using llvm::DIScope;
  using llvm::DISubprogram;
  using llvm::Value;

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
    linkageName.reserve(64);
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

    llvm::Type* funcType = _module->types().get(func->type());

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

      if (_module->isDebug()) {
        std::string linkageName;
        linkageName.reserve(64);
        getLinkageName(linkageName, func);
        // TODO: get this from the module
        DIScope* enclosingScope = _module->diCompileUnit();
        DIFile* diFile = _module->getDIFile(func->location().source);
        DISubprogram* sp = diBuilder().createFunction(
            enclosingScope, func->name(), linkageName, diFile, func->location().startLine,
            _module->diTypeBuilder().createFunctionType(func->type()),
            false /* internal linkage */, true /* definition */, 0,
            DINode::FlagPrototyped, false);
        _irFunction->setSubprogram(sp);
        setScope(sp);
        setDebugLocation(func->location());
      //   dbgInlineContext_ = DIScope();
      }

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

        BasicBlock * blkEntry = createBlock("entry");
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

  Value* CGFunctionBuilder::visitExpr(Expr* expr) {
    switch (expr->kind) {
      case Expr::Kind::BLOCK:
        return visitBlockStmt(static_cast<BlockStmt*>(expr));

      case Expr::Kind::RETURN:
        return visitReturnStmt(static_cast<ReturnStmt*>(expr));

      case Expr::Kind::ADD: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFAdd(lhs, rhs);
        } else {
          return _builder.CreateAdd(lhs, rhs);
        }
      }

      case Expr::Kind::SUBTRACT: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFSub(lhs, rhs);
        } else {
          return _builder.CreateSub(lhs, rhs);
        }
      }

      case Expr::Kind::MULTIPLY: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFMul(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          return _builder.CreateMul(lhs, rhs);
        }
      }

      case Expr::Kind::DIVIDE: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFDiv(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateUDiv(lhs, rhs);
          } else {
            return _builder.CreateSDiv(lhs, rhs);
          }
        } else {
          assert(false && "Invalid division type");
        }
      }

      case Expr::Kind::REMAINDER: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateURem(lhs, rhs);
          } else {
            return _builder.CreateSRem(lhs, rhs);
          }
        } else {
          assert(false && "Invalid remainder type");
        }
      }

      case Expr::Kind::EQ: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpOEQ(lhs, rhs);
        } else {
          return _builder.CreateICmpEQ(lhs, rhs);
        }
      }

      case Expr::Kind::NE: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpONE(lhs, rhs);
        } else {
          return _builder.CreateICmpNE(lhs, rhs);
        }
      }

      case Expr::Kind::LT: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpOLT(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateICmpULT(lhs, rhs);
          } else {
            return _builder.CreateICmpSLT(lhs, rhs);
          }
        } else {
          assert(false && "Invalid LT type");
        }
      }

      case Expr::Kind::LE: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpOLE(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateICmpULE(lhs, rhs);
          } else {
            return _builder.CreateICmpSLE(lhs, rhs);
          }
        } else {
          assert(false && "Invalid LT type");
        }
      }

      case Expr::Kind::GT: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpOGT(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateICmpUGT(lhs, rhs);
          } else {
            return _builder.CreateICmpSGT(lhs, rhs);
          }
        } else {
          assert(false && "Invalid LT type");
        }
      }

      case Expr::Kind::GE: {
        auto iop = static_cast<BinaryOp*>(expr);
        auto lhs = visitExpr(iop->args[0]);
        auto rhs = visitExpr(iop->args[1]);
        if (iop->type->kind == Type::Kind::FLOAT) {
          return _builder.CreateFCmpOGE(lhs, rhs);
        } else if (auto itype = dyn_cast<IntegerType>(iop->type)) {
          if (itype->isUnsigned()) {
            return _builder.CreateICmpUGE(lhs, rhs);
          } else {
            return _builder.CreateICmpSGE(lhs, rhs);
          }
        } else {
          assert(false && "Invalid LT type");
        }
      }

      case Expr::Kind::INTEGER_LITERAL:
        return visitIntegerLiteral(static_cast<IntegerLiteral*>(expr));
      default:
        assert(false && "Invalid expression type");
    }
  }

  Value* CGFunctionBuilder::visitBlockStmt(BlockStmt* blk) {
    // size_t savedRootCount = rootStackSize();
    // pushRoots(in->scope());

    // Establish a new lexical scope.
    DIScope* savedScope = nullptr;
    if (_module->isDebug() && blk->location.source) {
      savedScope = setScope(diBuilder().createLexicalBlock(
        _lexicalScope, _module->getDIFile(blk->location.source),
        blk->location.startLine, blk->location.startCol));
    }

    // Generate code for each statement in the block.
    for (auto it = blk->stmts.begin(); it != blk->stmts.end(); ++it) {
      if (atTerminator()) {
        diag.warn(*it) << "Unreachable statement";
      }
      setDebugLocation((*it)->location);
      if (!visitExpr(*it)) {
        //endLexicalBlock(savedScope);
        return NULL;
      }
    }

    llvm::Value* result = nullptr;
    if (blk->result) {
      setDebugLocation(blk->result->location);
      result = visitExpr(blk->result);
    }

    // Restore the lexical scope.
    if (savedScope) {
      setScope(savedScope);
    }

    // if (!atTerminator()) {
    //   popRootStack(savedRootCount);
    // }

    return result ? result : voidValue();
  }

  Value* CGFunctionBuilder::visitIfStmt(IfStmt* in) {
    // Create the set of basic blocks. We don't know yet
    // if we need an else block or endif block.
    BasicBlock * blkThen = createBlock("if.then");
    BasicBlock * blkElse = NULL;
    BasicBlock * blkThenLast = NULL;
    BasicBlock * blkElseLast = NULL;
    BasicBlock * blkDone = NULL;
    // size_t savedRootCount = rootStackSize();
    // pushRoots(in->scope());

    if (in->elseBlock != NULL) {
      blkElse = createBlock("else");
      setDebugLocation(in->test->location);
      genTestExpr(in->test, blkThen, blkElse);
    } else {
      blkDone = createBlock("endif");
      genTestExpr(in->test, blkThen, blkDone);
    }

    // Generate the contents of the 'then' block.
    moveToEnd(blkThen);
    _builder.SetInsertPoint(blkThen);
    setDebugLocation(in->thenBlock->location);
    Value * thenVal = visitExpr(in->thenBlock);
    Value * elseVal = NULL;

    // Only generate a branch if we haven't returned or thrown.
    if (!atTerminator()) {
      blkThenLast = _builder.GetInsertBlock();
      blkDone = ensureBlock("endif", blkDone);
      _builder.CreateBr(blkDone);
    }

    // Generate the contents of the 'else' block
    if (blkElse != NULL) {
      moveToEnd(blkElse);
      _builder.SetInsertPoint(blkElse);
      setDebugLocation(in->elseBlock->location);
      elseVal = visitExpr(in->elseBlock);

      // Only generate a branch if we haven't returned or thrown
      if (!atTerminator()) {
        blkElseLast = _builder.GetInsertBlock();
        blkDone = ensureBlock("endif", blkDone);
        _builder.CreateBr(blkDone);
      }
    }

    // Continue at the 'done' block if there was one.
    if (blkDone != NULL) {
      moveToEnd(blkDone);
      _builder.SetInsertPoint(blkDone);
      // popRootStack(savedRootCount);

      // If the if-statement was in expression context, then return the value.
      if (!isVoidType(in->type)) {
        if (blkThenLast && blkElseLast) {
          // If both branches returned a result, then combine them with a phi-node.
          assert(thenVal != NULL);
          assert(elseVal != NULL);
          llvm::PHINode* phi = _builder.CreatePHI(thenVal->getType(), 2, "if");
          phi->addIncoming(thenVal, blkThenLast);
          phi->addIncoming(elseVal, blkElseLast);
          return phi;
        } else if (blkThenLast) {
          assert(thenVal != NULL);
          return thenVal;
        } else if (blkElseLast) {
          assert(elseVal != NULL);
          return elseVal;
        } else {
          assert(false && "No value to return from if-statement");
        }
      }
    } else {
      assert(atTerminator());
    }

    return voidValue();
  }

  Value* CGFunctionBuilder::visitReturnStmt(ReturnStmt* in) {
    // Execute all cleanups
    // for (BlockExits * be = blockExits_; be != NULL; be = be->parent()) {
    //   if (be->cleanupBlock() != NULL) {
    //     callCleanup(be);
    //   }
    // }

    // setDebugLocation(in->location());
    if (in->expr == NULL) {
      // Return nothing
      _builder.CreateRet(NULL);
    } else {
      // Generate the return value.
      Expr* returnVal = in->expr;
      Value* value = visitExpr(returnVal);
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

  bool CGFunctionBuilder::genTestExpr(Expr* test, BasicBlock* blkTrue, BasicBlock* blkFalse) {
    switch (test->kind) {
      case Expr::Kind::LOGICAL_AND: {
        auto op = static_cast<const BinaryOp*>(test);
        BasicBlock * blkSecond = BasicBlock::Create(_gen.context, "and", _irFunction, blkTrue);
        genTestExpr(op->args[0], blkSecond, blkFalse);
        _builder.SetInsertPoint(blkSecond);
        genTestExpr(op->args[1], blkTrue, blkFalse);
        return true;
      }

      case Expr::Kind::LOGICAL_OR: {
        auto op = static_cast<const BinaryOp*>(test);
        BasicBlock * blkSecond = BasicBlock::Create(_gen.context, "or", _irFunction, blkFalse);
        genTestExpr(op->args[0], blkTrue, blkSecond);
        _builder.SetInsertPoint(blkSecond);
        genTestExpr(op->args[1], blkTrue, blkFalse);
        return true;
      }

      case Expr::Kind::NOT: {
        // For logical negation, flip the true and false blocks.
        auto op = static_cast<const UnaryOp*>(test);
        return genTestExpr(op->arg, blkFalse, blkTrue);
      }

      default: {
        llvm::Value* testVal = visitExpr(test);
        if (testVal == NULL) return false;
        _builder.CreateCondBr(testVal, blkTrue, blkFalse);
        return true;
      }
    }
  }

  void CGFunctionBuilder::moveToEnd(llvm::BasicBlock* blk) {
    llvm::BasicBlock* lastBlk = _builder.GetInsertBlock();
    if (lastBlk == NULL && _irFunction != NULL && !_irFunction->empty()) {
      lastBlk = &_irFunction->back();
    }
    if (lastBlk != NULL) {
      blk->moveAfter(lastBlk);
    }
  }

  Value* CGFunctionBuilder::visitIntegerLiteral(IntegerLiteral* value) {
    return llvm::Constant::getIntegerValue(_module->types().get(value->type), value->value());
  }

  Value* CGFunctionBuilder::voidValue() const {
    return llvm::UndefValue::get(llvm::Type::getVoidTy(_gen.context));
  }

  llvm::BasicBlock* CGFunctionBuilder::createBlock(const llvm::Twine& blkName) {
    return BasicBlock::Create(_gen.context, blkName, _irFunction);
  }

  void CGFunctionBuilder::setDebugLocation(const Location& loc) {
    if (loc.source) {
      _builder.SetCurrentDebugLocation(
          llvm::DebugLoc::get(loc.startLine, loc.endLine, _lexicalScope));
    }
  }

  bool CGFunctionBuilder::atTerminator() const {
    return _builder.GetInsertBlock() == NULL || _builder.GetInsertBlock()->getTerminator() != NULL;
  }
}
