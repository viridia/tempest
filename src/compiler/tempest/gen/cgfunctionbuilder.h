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
  class ReturnStmt;
  class IntegerLiteral;
}

namespace tempest::gen {
  using namespace tempest::sema::graph;

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

    /** Type factory. */
    CGTypeBuilder& types();

  private:
    CodeGen& _gen;
    CGModule* _module;
    llvm::IRBuilder<> _builder;
    llvm::Module* _irModule;
    llvm::Function* _irFunction;

    llvm::Value* visitExpr(Expr* expr);
    llvm::Value* visitBlockStmt(BlockStmt* blk);
    llvm::Value* visitReturnStmt(ReturnStmt* ret);
    llvm::Value* visitIntegerLiteral(IntegerLiteral* ret);

    llvm::Value* voidValue() const;
    bool atTerminator() const;
  };
}

#endif
