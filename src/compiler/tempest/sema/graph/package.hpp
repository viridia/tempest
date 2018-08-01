// ============================================================================
// semgraph/pacakge.h: Semantic graph nodes for packages.
// ============================================================================

#ifndef TEMPEST_SEMA_GRAPH_PACKAGE_HPP
#define TEMPEST_SEMA_GRAPH_PACKAGE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

namespace tempest::sema::graph {

class Module;

  // FIXME: Packages no longer exist in the semantic graph, we can delete this.
  // class Package : public Member {
  // public:
  //   Package(const llvm::StringRef& name, Member* definedIn = nullptr)
  //     : Member(Kind::PACKAGE, name)
  //     , _definedIn(definedIn)
  //     , _memberScope(std::make_unique<SymbolTable>())
  //   {}

  //   // Package(const llvm::StringRef& name, Member* definedIn = NULL)
  //   //   : Member(Kind::PACKAGE, name, definedIn)
  //   // {}

  //   /** Add a module to this package. */
  //   void addModule(Module* module);

  //   /** Symbol scope for this package's members. */
  //   SymbolTable* memberScope() const { return _memberScope.get(); }
  //   void setMemberScope(SymbolTable* scope) { _memberScope.reset(scope); }

  //   /** The absolute path to this package on disk. May be empty if the package was loaded from
  //       a library. */
  //   const llvm::StringRef path() const { return _path; }

  //   void format(std::ostream& out) const {}

  //   /** Dynamic casting support. */
  //   static bool classof(const Package* m) { return true; }
  //   static bool classof(const Member* m) { return m->kind == Kind::PACKAGE; }

  // private:
  //   Member* _definedIn;
  //   std::unique_ptr<SymbolTable> _memberScope;
  //   std::string _path;
  // };

}

#endif
