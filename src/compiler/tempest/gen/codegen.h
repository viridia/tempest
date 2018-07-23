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
  // using tempest::sema::graph::Type;

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
      , _types(context, _alloc)
    {}

    /** The current module. */
    CGModule* module() { return _module; }

    /** Type factory instance for this module. */
    CGTypeBuilder& types() { return _types; }

    /** Temporary storage for code generation. */
    llvm::BumpPtrAllocator& alloc() { return _alloc; }

    /** Create a new module. */
    CGModule* createModule(llvm::StringRef name);

    /** Generate a declaration for a function in the current module. */
    llvm::Function* genFunctionValue(FunctionDefn* func);

    /** Generate a definition for a function in the current module. */
    llvm::Function* genFunction(FunctionDefn* func);

    /** Set the current type arguments for the definition being generated. Returns the previous
        type arguments. */
    // void setTypeArgs(const llvm::ArrayRef<Type*>& args, llvm::SmallVectorBase<Type*>& oldArgs);

    // /** Restore the type arguments to their previous values. */
    // void restoreTypeArgs(llvm::SmallVectorBase<Type*>& oldArgs);

    // visitModule

  private:
    CGModule* _module;
    CGTypeBuilder _types;
    llvm::BumpPtrAllocator _alloc;
  };
}

#endif
