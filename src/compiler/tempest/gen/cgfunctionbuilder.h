#ifndef TEMPEST_GEN_CGFUNCTIONBUILDER_H
#define TEMPEST_GEN_CGFUNCTIONBUILDER_H 1

#ifndef TEMPEST_GEN_CODEGEN_H
  #include "tempest/gen/codegen.h"
#endif

#ifndef TEMPEST_GEN_CGMODULE_H
  #include "tempest/gen/cgmodule.h"
#endif

#ifndef LLVM_IR_IRBUILDER_H
  #include <llvm/IR/IRBuilder.h>
#endif

#ifndef LLVM_IR_FUNCTION_H
  #include <llvm/IR/Function.h>
#endif

namespace tempest::sema::graph {
  class BlockStmt;
  class IfStmt;
  class ReturnStmt;
  class IntegerLiteral;
}

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::error::Location;

  class CGModule;
  class CGTypeBuilder;

  /** Basic block in the code flow graph. */
  class CGFunctionBuilder {
  public:
    CGFunctionBuilder(CodeGen& codeGen, CGModule* module);
    CGFunctionBuilder(const CGFunctionBuilder&) = delete;
    ~CGFunctionBuilder() {}

    /** Generate a declaration for a function. */
    llvm::Function* genFunctionValue(FunctionDefn* func);

    /** Generate a definition for a function. */
    llvm::Function* genFunction(FunctionDefn* func);

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
    llvm::IRBuilder<> _builder;
    llvm::Module* _irModule;
    llvm::Function* _irFunction;
    llvm::DIScope* _lexicalScope;

    llvm::Value* visitExpr(Expr* expr);
    llvm::Value* visitBlockStmt(BlockStmt* blk);
    llvm::Value* visitIfStmt(IfStmt* in);
    llvm::Value* visitReturnStmt(ReturnStmt* ret);
    llvm::Value* visitIntegerLiteral(IntegerLiteral* ret);

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
