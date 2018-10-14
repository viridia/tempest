#ifndef TEMPEST_GEN_CGFUNCTIONBUILDER_HPP
#define TEMPEST_GEN_CGFUNCTIONBUILDER_HPP 1

#ifndef TEMPEST_GEN_CODEGEN_HPP
  #include "tempest/gen/codegen.hpp"
#endif

#ifndef TEMPEST_GEN_CGMODULE_HPP
  #include "tempest/gen/cgmodule.hpp"
#endif

#ifndef LLVM_IR_IRBUILDER_H
  #include <llvm/IR/IRBuilder.h>
#endif

#ifndef LLVM_IR_FUNCTION_H
  #include <llvm/IR/Function.h>
#endif

namespace tempest::sema::graph {
  class ApplyFnOp;
  class BlockStmt;
  class IfStmt;
  class IntegerLiteral;
  class ReturnStmt;
  class SymbolRefExpr;
}

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::error::Location;

  class CGModule;
  class CGTypeBuilder;
  class ClassDescriptorSym;

  /** Basic block in the code flow graph. */
  class CGFunctionBuilder {
  public:
    CGFunctionBuilder(CodeGen& codeGen, CGModule* module, ArrayRef<const Type*> typeArgs);
    CGFunctionBuilder(const CGFunctionBuilder&) = delete;
    ~CGFunctionBuilder() {}

    /** Generate a declaration for a function. */
    llvm::Function* genFunctionValue(FunctionDefn* func);

    /** Generate a definition for a function. */
    llvm::Function* genFunction(FunctionDefn* func, Expr* body);

    /** Convenience function to get the DIBuilder. */
    llvm::DIBuilder& diBuilder() const {
      return _module->diBuilder();
    }

    /** Set the current lexical scope. */
    llvm::DIScope* setScope(llvm::DIScope* newScope) {
      auto prev = _lexicalScope;
      _lexicalScope = newScope;
      return prev;
    }

    /** Set the current source position. */
    void setDebugLocation(const Location& loc);

  private:
    CodeGen& _gen;
    CGModule* _module;
    ArrayRef<const Type*> _typeArgs;
    llvm::IRBuilder<> _builder;
    llvm::Module* _irModule;
    llvm::Function* _irFunction = nullptr;
    llvm::DIScope* _lexicalScope = nullptr;

    llvm::Value* visitExpr(Expr* expr);
    llvm::Value* visitBlockStmt(BlockStmt* blk);
    llvm::Value* visitIfStmt(IfStmt* in);
    llvm::Value* visitReturnStmt(ReturnStmt* ret);
    llvm::Value* visitIntegerLiteral(IntegerLiteral* ret);
    llvm::Value* visitCall(ApplyFnOp* in);
    llvm::Value* visitCallIntrinsic(ApplyFnOp* in);
    llvm::Value* visitAllocObj(SymbolRefExpr* in);

    llvm::Value* genCallInstr(
        llvm::Value* func,
        ArrayRef<llvm::Value *> args,
        const llvm::Twine& name);

    /** Generate a conditional branch based on a test expression. Handles inlining of logical
        operators. */
    bool genTestExpr(Expr* test, llvm::BasicBlock* blkTrue, llvm::BasicBlock* blkFalse);

    /** Create a new basic block and append it to the current function. */
    llvm::BasicBlock* createBlock(const llvm::Twine& blkName);

    /** Create a new basic block and append it to the current function, but only if it doesn't
        already exist, otherwise just return the existing block. */
    llvm::BasicBlock* ensureBlock(const llvm::Twine & blkName, llvm::BasicBlock* blk) {
      return blk ? blk : createBlock(blkName);
    }

    /** Move the block 'blk' to the current insertion point, or to the end of the function
        if the insertion point is clear. */
    void moveToEnd(llvm::BasicBlock* blk);

    bool isVoidType(const Type* type) {
      return type->kind == Type::Kind::VOID;
    }

    llvm::Value* voidValue() const;
    bool atTerminator() const;
  };
}

#endif
