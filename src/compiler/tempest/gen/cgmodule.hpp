#ifndef TEMPEST_GEN_CGMODULE_HPP
#define TEMPEST_GEN_CGMODULE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_HPP
  #include "tempest/sema/graph/module.hpp"
#endif

#ifndef TEMPEST_GEN_CGTYPEBUILDER_HPP
  #include "tempest/gen/cgtypebuilder.hpp"
#endif

#ifndef TEMPEST_GEN_CGDEBUGTYPEBUILDER_HPP
  #include "tempest/gen/cgdebugtypebuilder.hpp"
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

#ifndef LLVM_IR_DIBUILDER_H
  #include <llvm/IR/DIBuilder.h>
#endif

#include <vector>

namespace tempest::gen {
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Module;
  using tempest::source::ProgramSource;

  class CGGlobal;
  class CGFunction;
  class CGStringLiteral;
  class ClassDescriptorSym;
  class InterfaceDescriptorSym;
  class ClassInterfaceTranslationSym;
  class GlobalVarSym;

  /** Code gen node for a compilation unit. Might include more than one source module. */
  class CGModule {
  public:
    CGModule(llvm::StringRef name, llvm::LLVMContext& context);

    /** Initialized the debug info. */
    void diInit(llvm::StringRef fileName, llvm::StringRef dirName);
    void diFinalize();

    /** The LLVM Module. */
    llvm::Module* irModule() { return _irModule.get(); }

    /** The DebugInfo builder. */
    llvm::DIBuilder& diBuilder() { return _diBuilder; }

    /** The IR Type builder. */
    CGTypeBuilder& types() { return _types; }

    /** The DebugInfo Type builder. */
    CGDebugTypeBuilder& diTypeBuilder() { return _diTypeBuilder; }

    /** The DebugInfo compile unit. */
    llvm::DICompileUnit* diCompileUnit() { return _diCompileUnit; }

    /** Allocator for this module. */
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    /** List of globals in this module. */
    std::vector<CGGlobal*>& globals() { return _globals; }
    const std::vector<CGGlobal*>& globals() const { return _globals; }

    /** List of functions in this module. */
    std::vector<CGFunction*>& functions() { return _functions; }
    const std::vector<CGFunction*>& functions() const { return _functions; }

    /** If true, means we're generating debug info. */
    bool isDebug() const { return _debug; }

    /** Generate code to call function fdef as the main entry point. */
    void makeEntryPoint(FunctionDefn* fdef);

    /** Get a file from a program source. */
    llvm::DIFile* getDIFile(ProgramSource* src);

    llvm::Value* genVarValue(GlobalVarSym* vsym);
    llvm::Constant* genGlobalVar(GlobalVarSym* vsym);

    /** Generate static class descriptor struct. */
    llvm::GlobalVariable* genClassDescValue(ClassDescriptorSym* clsSym);
    llvm::GlobalVariable* genClassDesc(ClassDescriptorSym* clsSym);

    /** Generate static interface descriptor struct. */
    llvm::GlobalVariable* genInterfaceDescValue(InterfaceDescriptorSym* clsSym);
    llvm::GlobalVariable* genInterfaceDesc(InterfaceDescriptorSym* clsSym);

    /** Generate class to interface mapping. */
    llvm::GlobalVariable* genClassInterfaceTransValue(ClassInterfaceTranslationSym* clsSym);
    llvm::GlobalVariable* genClassInterfaceTrans(ClassInterfaceTranslationSym* clsSym);

    /** Return a reference to the allocator function for garbage-collected memory. */
    llvm::Function* getGCAlloc();

  private:
    llvm::LLVMContext& _context;
    std::unique_ptr<llvm::Module> _irModule;
    llvm::DIBuilder _diBuilder;
    llvm::DICompileUnit* _diCompileUnit;
    tempest::support::BumpPtrAllocator _alloc;
    llvm::StringMap<CGStringLiteral*> _stringLiterals;
    std::vector<CGGlobal*> _globals;
    std::vector<CGFunction*> _functions;
    CGTypeBuilder _types;
    CGDebugTypeBuilder _diTypeBuilder;
    llvm::Function* _gcAlloc = nullptr;
    bool _debug;
  };
}

#endif
