#ifndef TEMPEST_SEMA_NAMES_MEMBERNAMELOOKUP_HPP
#define TEMPEST_SEMA_NAMES_MEMBERNAMELOOKUP_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_SMALLPTRSET_H
  #include <llvm/ADT/SmallPtrSet.h>
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

// namespace spark {
// namespace error { class Reporter; }
// namespace semgraph { class Member; class Type; }

namespace tempest::sema::names {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::NameCallback;
  using tempest::sema::graph::NameLookupResultRef;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;

  /** Name resolver specialized for resolving types. */
  class MemberNameLookup {
  public:
    /** Given a list of members to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const llvm::ArrayRef<Member*>& stem,
        bool fromStatic,
        NameLookupResultRef& result);

    /** Given a list of types to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const llvm::ArrayRef<const Type*>& stem,
        bool fromStatic,
        NameLookupResultRef& result);

    /** Given a member to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const Member* stem,
        bool fromStatic,
        NameLookupResultRef& result);

    /** Given a type to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const Type* stem,
        bool fromStatic,
        NameLookupResultRef& result);

    /** Iterate through all names. */
    void forAllNames(const llvm::ArrayRef<Member*>& stem, const NameCallback& nameFn);
    void forAllNames(Member* stem, const NameCallback& nameFn);
    void forAllNames(Type* stem, const NameCallback& nameFn);

  private:
    void lookupInherited(
        const llvm::StringRef& name,
        UserDefinedType* stem,
        bool fromStatic,
        NameLookupResultRef& result);
  };
}

#endif
