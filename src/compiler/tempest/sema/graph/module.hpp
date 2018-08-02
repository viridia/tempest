#ifndef TEMPEST_SEMA_GRAPH_MODULE_HPP
#define TEMPEST_SEMA_GRAPH_MODULE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SOURCE_PROGRAMSOURCE_HPP
  #include "tempest/source/programsource.hpp"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

namespace tempest::sema::graph {

  /** Semantic graph node for a module. */
  class Module : public Member {
  public:
    Module(
        std::unique_ptr<source::ProgramSource> source,
        const llvm::StringRef& name)
      : Member(Kind::MODULE, name)
      , _source(std::move(source))
      , _memberScope(std::make_unique<SymbolTable>())
      , _importScope(std::make_unique<SymbolTable>())
    {}

    // Constructor for testing, program source is null.
    Module(const llvm::StringRef& name)
      : Member(Kind::MODULE, name)
      , _memberScope(std::make_unique<SymbolTable>())
      , _importScope(std::make_unique<SymbolTable>())
    {}

    /** Source file of this module. */
    source::ProgramSource* source() { return _source.get(); }
    const source::ProgramSource* source() const { return _source.get(); }

    /** Members of this module. */
    std::vector<Defn*>& members() { return _members; }
    const std::vector<Defn*>& members() const { return _members; }

    /** Symbol scope for this module's members. */
    SymbolTable* memberScope() const { return _memberScope.get(); }

    /** Symbol scope for this module's imports. */
    SymbolTable* importScope() const { return _importScope.get(); }

    /** Allocator used for AST nodes. */
    llvm::BumpPtrAllocator& astAlloc() { return _astAlloc; }

    /** Allocator used for semantic graph. */
    llvm::BumpPtrAllocator& semaAlloc() { return _semaAlloc; }

    /** Dynamic casting support. */
    static bool classof(const Module* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::MODULE; }

  private:
    std::unique_ptr<source::ProgramSource> _source;
    std::vector<Defn*> _members;
    MemberList _imports;
    std::unique_ptr<SymbolTable> _memberScope;
    std::unique_ptr<SymbolTable> _importScope;
    llvm::BumpPtrAllocator _astAlloc;
    llvm::BumpPtrAllocator _semaAlloc;
    // std::unordered_set<SpecializedDefn*> _syntheticFunctions;
  };
}

#endif
