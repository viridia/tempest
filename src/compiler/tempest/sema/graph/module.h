// ============================================================================
// sema/graph/module.h: Semantic graph nodes for modules.
// ============================================================================

#ifndef TEMPEST_SEMA_GRAPH_MODULE_H
#define TEMPEST_SEMA_GRAPH_MODULE_H 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

namespace tempest::sema::graph {

  /** Semantic graph node for a module. */
  class Module : public Member {
  public:
    Module(
        const source::ProgramSource* source,
        const llvm::StringRef& name,
        Member* definedIn = NULL)
      : Member(Kind::MODULE, name)
      , _source(source)
      , _definedIn(definedIn)
      , _memberScope(std::make_unique<SymbolTable>())
      , _importScope(std::make_unique<SymbolTable>())
    {}

    /** Source file of this module. */
    const source::ProgramSource* source() { return _source; }

    /** Members of this module. */
    MemberList& members() { return _members; }
    const MemberList& members() const { return _members; }

    /** Symbol scope for this module's members. */
    SymbolTable* memberScope() const { return _memberScope.get(); }

    /** Symbol scope for this module's imports. */
    SymbolTable* importScope() const { return _importScope.get(); }

    /** Package in which this module was defined. */
    Member* definedIn() const { return _definedIn; }

    /** Allocator used for AST nodes. */
    llvm::BumpPtrAllocator& astAlloc() { return _astAlloc; }

    /** Allocator used for semantic graph. */
    llvm::BumpPtrAllocator& semaAlloc() { return _semaAlloc; }

    /** The absolute path to this module on disk. May be empty if the package was loaded from
        a library. */
    const llvm::StringRef path() const { return _path; }

    void format(std::ostream& out) const {}

    /** Dynamic casting support. */
    static bool classof(const Module* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::MODULE; }

  private:
    const source::ProgramSource* _source;
    Member* _definedIn;
    MemberList _members;
    MemberList _imports;
    std::unique_ptr<SymbolTable> _memberScope;
    std::unique_ptr<SymbolTable> _importScope;
    std::string _path;
    llvm::BumpPtrAllocator _astAlloc;
    llvm::BumpPtrAllocator _semaAlloc;
    // References to Specialized functions
    // References to Type Descriptors
    // std::unordered_set<SpecializedDefn*> _syntheticFunctions;
  };
}

#endif
