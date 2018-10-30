#ifndef TEMPEST_GEN_CODEGEN_HPP
#define TEMPEST_GEN_CODEGEN_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_HPP
  #include "tempest/sema/graph/module.hpp"
#endif

#ifndef TEMPEST_GEN_CGTYPEBUILDER_HPP
  #include "tempest/gen/cgtypebuilder.hpp"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#ifndef TEMPEST_ERROR_REPORTER_HPP
  #include "tempest/error/reporter.hpp"
#endif

#ifndef LLVM_IR_FUNCTION_H
  #include <llvm/IR/Function.h>
#endif

namespace tempest::gen {
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Module;

  class CGFunction;
  class CGModule;
  class CGStringLiteral;
  class CGTarget;
  class SymbolStore;

  /** Code gen node for a compilation unit. Might include more than one source module. */
  class CodeGen {
  public:
    llvm::LLVMContext& context;

    CodeGen(llvm::LLVMContext& context, CGTarget& target)
      : context(context)
      , _target(target)
      , _module(nullptr)
    {}

    /** The current module. */
    CGModule* module() { return _module; }

    /** Create a new module. */
    CGModule* createModule(llvm::StringRef name);

    /** Generate all symbols in the symbol store. */
    void genSymbols(SymbolStore& symbols);

    /** Generate a declaration for a function in the current module. */
    llvm::Function* genFunctionValue(FunctionDefn* func, ArrayRef<const Type*> typeArgs);

    /** Generate a definition for a function in the current module. */
    llvm::Function* genFunction(FunctionDefn* func, ArrayRef<const Type*> typeArgs);

  private:
    CGTarget& _target;
    CGModule* _module;
  };
}

#endif
