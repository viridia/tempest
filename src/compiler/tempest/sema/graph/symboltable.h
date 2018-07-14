// ============================================================================
// scope/scope.h: Symbol table scopes.
// ============================================================================

#ifndef TEMPEST_GRAPH_SYMBOLTABLE_H
#define TEMPEST_GRAPH_SYMBOLTABLE_H 1

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_H
  #include "tempest/sema/graph/member.h"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
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

  /** A symbol table. */
  class SymbolTable {
  public:
    virtual ~SymbolTable() {}

    /** Add a member to this scope. Note that many scope implementations don't allow this. */
    virtual void addMember(Member* m);

    /** Lookup a name, and produce a list of results for that name. */
    virtual void lookupName(const llvm::StringRef& name, MemberList &result) const;

    /** Call the specified functor for all names defined in this scope. */
    virtual void forAllNames(const NameCallback& nameFn) const;

  private:
    typedef llvm::StringMap<std::vector<Member*>> EntryMap;

    EntryMap _entries;
  };
}

#endif
