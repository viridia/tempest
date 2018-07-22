#ifndef TEMPEST_GEN_CGMODULE_H
#define TEMPEST_GEN_CGMODULE_H 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_H
  #include "tempest/sema/graph/module.h"
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#ifndef LLVM_IR_MODULE_H
  #include <llvm/IR/Module.h>
#endif

#include <vector>

namespace tempest::gen {
  using tempest::sema::graph::Module;

  class CGGlobal;
  class CGFunction;
  class CGStringLiteral;

  /** Code gen node for a compilation unit. Might include more than one source module. */
  class CGModule {
  public:
    CGModule(llvm::StringRef name, llvm::LLVMContext& context);

    /** The LLVM Module. */
    llvm::Module* irModule() { return _irModule.get(); }

    /** Allocator for this module. */
    llvm::BumpPtrAllocator& alloc() { return _alloc; }

    /** List of globals in this module. */
    std::vector<CGGlobal*>& globals() { return _globals; }
    const std::vector<CGGlobal*>& globals() const { return _globals; }

    /** List of functions in this module. */
    std::vector<CGFunction*>& functions() { return _functions; }
    const std::vector<CGFunction*>& functions() const { return _functions; }

  private:
    std::unique_ptr<llvm::Module> _irModule;
    llvm::BumpPtrAllocator _alloc;
    llvm::StringMap<CGStringLiteral*> _stringLiterals;
    std::vector<CGGlobal*> _globals;
    std::vector<CGFunction*> _functions;
  };
}

// AliasListType AliasList;        ///< The Aliases in the module
// IFuncListType IFuncList;        ///< The IFuncs in the module
// NamedMDListType NamedMDList;    ///< The named metadata in the module
// ValueSymbolTable *ValSymTab;    ///< Symbol table for values
// std::unique_ptr<GVMaterializer>
// Materializer;                   ///< Used to materialize GlobalValues
// std::string TargetTriple;       ///< Platform target triple Module compiled on
//                                 ///< Format: (arch)(sub)-(vendor)-(sys0-(abi)
// void *NamedMDSymTab;            ///< NamedMDNode names.
// DataLayout DL;                  ///< DataLayout associated with the module

#endif
