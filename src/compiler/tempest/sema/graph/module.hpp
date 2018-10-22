#ifndef TEMPEST_SEMA_GRAPH_MODULE_HPP
#define TEMPEST_SEMA_GRAPH_MODULE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_EXTENSIONMAP_HPP
  #include "tempest/sema/graph/extensionmap.hpp"
#endif

#ifndef TEMPEST_SOURCE_PROGRAMSOURCE_HPP
  #include "tempest/source/programsource.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::ast {
  class Module;
}

namespace tempest::sema::graph {

  /** Indicates where this modules was loaded from, which determines how it will be processed. */
  enum class ModuleGroup {
    UNSET = 0,
    SOURCE,           // It's a source file to be compiled.
    IMPORT_SOURCE,    // It's a source file that was imported, needs to be analyzed.
    IMPORT_COMPILED,  // It's an imported module that has already been compiled.
  };

  /** Semantic graph node for a module. */
  class Module : public Member {
  public:
    Module(
        std::unique_ptr<source::ProgramSource> source,
        const llvm::StringRef& name)
      : Member(Kind::MODULE, name)
      , _source(std::move(source))
      , _memberScope(std::make_unique<SymbolTable>())
      , _exportScope(std::make_unique<SymbolTable>())
      , _extensions(std::make_unique<ExtensionMap>())
    {}

    // Constructor for testing, program source is null.
    Module(const llvm::StringRef& name)
      : Member(Kind::MODULE, name)
      , _memberScope(std::make_unique<SymbolTable>())
      , _exportScope(std::make_unique<SymbolTable>())
      , _extensions(std::make_unique<ExtensionMap>())
    {}

    /** Source file of this module. */
    source::ProgramSource* source() { return _source.get(); }
    const source::ProgramSource* source() const { return _source.get(); }

    /** Which processing group this module is in. */
    ModuleGroup group() const { return _group; }
    void setGroup(ModuleGroup group) { _group = group; }

    /** AST for this module. */
    const ast::Module* ast() const { return _ast; }
    void setAst(const ast::Module* ast) { _ast = ast; }

    /** Members of this module. */
    DefnList& members() { return _members; }
    const DefnArray members() const { return _members; }

    /** Symbol scope for this module's members. */
    SymbolTable* memberScope() const { return _memberScope.get(); }

    /** Symbol scope for the symbols exported by this module. */
    SymbolTable* exportScope() const { return _exportScope.get(); }

    /** Extension map for this module. */
    ExtensionMap* extensions() const { return _extensions.get(); }

    /** Allocator used for AST nodes. */
    tempest::support::BumpPtrAllocator& astAlloc() { return _astAlloc; }

    /** Allocator used for semantic graph. */
    tempest::support::BumpPtrAllocator& semaAlloc() { return _semaAlloc; }

    /** Dynamic casting support. */
    static bool classof(const Module* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::MODULE; }

  private:
    std::unique_ptr<source::ProgramSource> _source;
    ModuleGroup _group = ModuleGroup::UNSET;
    const ast::Module* _ast = nullptr;
    DefnList _members;
    MemberList _imports;
    std::unique_ptr<SymbolTable> _memberScope;
    std::unique_ptr<SymbolTable> _exportScope;
    std::unique_ptr<ExtensionMap> _extensions;
    tempest::support::BumpPtrAllocator _astAlloc;
    tempest::support::BumpPtrAllocator _semaAlloc;
  };
}

#endif
