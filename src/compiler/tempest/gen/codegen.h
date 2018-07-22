#ifndef TEMPEST_GEN_CODEGEN_H
#define TEMPEST_GEN_CODEGEN_H 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

#ifndef TEMPEST_GEN_TYPES_CGTYPEFACTORY_H
  #include "tempest/gen/types/cgtypefactory.h"
#endif

// #ifndef TEMPEST_SEMA_GRAPH_MODULE_H
//   #include "tempest/sema/graph/module.h"
// #endif

// #ifndef LLVM_ADT_SMALLVECTOR_H
//   #include <llvm/ADT/SmallVector.h>
// #endif

// #ifndef LLVM_ADT_STRINGMAP_H
//   #include <llvm/ADT/StringMap.h>
// #endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

// #ifndef LLVM_IR_MODULE_H
//   #include <llvm/IR/Module.h>
// #endif

// #include <vector>

namespace tempest::gen {
  using tempest::sema::graph::Module;

  class CGFunction;
  class CGStringLiteral;

  /** Code gen node for a compilation unit. Might include more than one source module. */
  class CodeGen {
  public:
    CodeGen(llvm::StringRef name, llvm::LLVMContext& context)
      : _types(context, _alloc)
    {}

    /** The current module. */
    CGModule* module() { return _module.get(); }

    /** Type factory instance for this module. */
    types::CGTypeFactory& types() { return _types; }

    /** Temporary storage for code generation. */
    llvm::BumpPtrAllocator& alloc() { return _alloc; }

    /** Set the current type arguments for the definition being generated. Returns the previous
        type arguments. */
    void setTypeArgs(const llvm::ArrayRef<Type*>& args, llvm::SmallVectorBase<Type*>& oldArgs);

    /** Restore the type arguments to their previous values. */
    void restoreTypeArgs(llvm::SmallVectorBase<Type*>& oldArgs);

    // visitModule

  private:
    std::unique_ptr<CGModule> _module;
    types::CGTypeFactory _types;
    llvm::BumpPtrAllocator _alloc;
  };
}

#endif
