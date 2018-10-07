#ifndef TEMPEST_SEMA_NAMES_MEMBERNAMELOOKUP_HPP
#define TEMPEST_SEMA_NAMES_MEMBERNAMELOOKUP_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

#ifndef LLVM_ADT_SMALLPTRSET_H
  #include <llvm/ADT/SmallPtrSet.h>
#endif

namespace tempest::sema::graph {
  class SpecializationStore;
}

namespace tempest::sema::names {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::NameCallback;
  using tempest::sema::graph::MemberAndStem;
  using tempest::sema::graph::MemberLookupResultRef;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;

  /** Name resolver specialized for resolving types. */
  class MemberNameLookup {
  public:
    enum Flags {
      STATIC_MEMBERS = (1 << 0),
      INSTANCE_MEMBERS = (1 << 1),
      INHERITED_ONLY = (1 << 2),
    };

    MemberNameLookup(graph::SpecializationStore& specs) : _specs(specs) {}

    /** Given a list of members to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const llvm::ArrayRef<MemberAndStem>& stem,
        MemberLookupResultRef& result,
        size_t flags = INSTANCE_MEMBERS);

    /** Given a list of types to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const llvm::ArrayRef<const Type*>& stem,
        MemberLookupResultRef& result,
        size_t flags = INSTANCE_MEMBERS);

    /** Given a member to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const Member* stem,
        MemberLookupResultRef& result,
        size_t flags = INSTANCE_MEMBERS);

    /** Given a type to look in, find members with the specified name. */
    void lookup(
        const llvm::StringRef& name,
        const Type* stem,
        MemberLookupResultRef& result,
        size_t flags = INSTANCE_MEMBERS);

    /** Iterate through all names. */
    void forAllNames(const llvm::ArrayRef<Member*>& stem, const NameCallback& nameFn);
    void forAllNames(Member* stem, const NameCallback& nameFn);
    void forAllNames(Type* stem, const NameCallback& nameFn);

  private:
    graph::SpecializationStore& _specs;

    void lookupInherited(
        const llvm::StringRef& name,
        UserDefinedType* stem,
        MemberLookupResultRef& result,
        size_t flags);
  };
}

#endif
