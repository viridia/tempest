// ============================================================================
// scope/scope.h: Symbol table scopes.
// ============================================================================

#ifndef TEMPEST_GRAPH_SYMBOLTABLE_HPP
#define TEMPEST_GRAPH_SYMBOLTABLE_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

#include <functional>
#include <ostream>
#include <vector>
#include <unordered_map>

namespace tempest::sema::graph {
  /** Lambda expression type for name callbacks. */
  typedef std::function<void (const llvm::StringRef&)> NameCallback;

  typedef llvm::SmallVectorImpl<Member*> NameLookupResultRef;
  typedef llvm::SmallVector<Member*, 8> NameLookupResult;

  /** A symbol table. */
  class SymbolTable {
  public:
    /** Add a member to this scope. Note that many scope implementations don't allow this. */
    void addMember(Member* m);

    /** Add a member to this scope with a different name. Use for import aliases. */
    void addMember(const llvm::StringRef& name, Member* m);

    /** Lookup a name, and produce a list of results for that name. */
    void lookupName(const llvm::StringRef& name, NameLookupResultRef &result) const;

    /** True if the given name is already defined, and include the location of the first
        definition. */
    bool exists(const llvm::StringRef& name, source::Location &location) const;

    /** Call the specified functor for all names defined in this scope. */
    void forAllNames(const NameCallback& nameFn) const;

  private:
    typedef llvm::StringMap<std::vector<Member*>> EntryMap;

    EntryMap _entries;
  };
}

#endif
