#ifndef TEMPEST_GEN_CODEGEN_H
#define TEMPEST_GEN_CODEGEN_H 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_H
  #include "tempest/sema/graph/module.h"
#endif

#ifndef TEMPEST_GEN_CGTYPEBUILDER_H
  #include "tempest/gen/cgtypebuilder.h"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#ifndef TEMPEST_ERROR_REPORTER_H
  #include "tempest/error/reporter.h"
#endif

#ifndef LLVM_IR_FUNCTION_H
  #include <llvm/IR/Function.h>
#endif

// #include <vector>

namespace tempest::gen {
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Module;

  class CGFunction;
  class CGModule;
  class CGStringLiteral;

  /** Code gen node for a compilation unit. Might include more than one source module. */
  class CodeGen {
  public:
    llvm::LLVMContext& context;

    CodeGen(llvm::LLVMContext& context)
      : context(context)
      , _module(nullptr)
    {}

    /** The current module. */
    CGModule* module() { return _module; }

    /** Create a new module. */
    CGModule* createModule(llvm::StringRef name);

    /** Generate a declaration for a function in the current module. */
    llvm::Function* genFunctionValue(FunctionDefn* func);

    /** Generate a definition for a function in the current module. */
    llvm::Function* genFunction(FunctionDefn* func);

  private:
    CGModule* _module;
  };
}

#endif
