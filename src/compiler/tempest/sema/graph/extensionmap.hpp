#ifndef TEMPEST_GRAPH_EXTENSIONMAP_HPP
#define TEMPEST_GRAPH_EXTENSIONMAP_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

namespace tempest::sema::graph {
  class TypeDefn;

  /** An extension map maintains a map of definitions to the list of 'extends' declarations. */
  class ExtensionMap {
  public:
    /** Add a member to this scope. Note that many scope implementations don't allow this. */
    void add(TypeDefn* original, TypeDefn* extension);

    /** Lookup a name, and produce a list of results for that name. */
    llvm::ArrayRef<TypeDefn*> lookup(TypeDefn* key) const;

  private:
    typedef std::unordered_map<TypeDefn*, std::vector<TypeDefn*>> EntryMap;

    EntryMap _entries;
  };
}

#endif
