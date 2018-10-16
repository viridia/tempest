#include "tempest/sema/names/membernamelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/specstore.hpp"
#include "tempest/sema/graph/symboltable.hpp"

#include <cassert>

namespace tempest::sema::names {
  using tempest::sema::graph::Defn;
  using tempest::sema::graph::Module;
  using tempest::sema::graph::PrimitiveType;
  using tempest::sema::graph::SpecializedDefn;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::TypeParameter;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::MemberLookupResult;
  using tempest::sema::graph::MemberLookupResultRef;

  void MemberNameLookup::lookup(
      const llvm::StringRef& name,
      const llvm::ArrayRef<MemberAndStem>& stem,
      MemberLookupResultRef& result,
      size_t flags) {
    for (auto& m : stem) {
      lookup(name, m.member, result, flags);
    }
  }

  void MemberNameLookup::lookup(
      const llvm::StringRef& name,
      const llvm::ArrayRef<const Type*>& stem,
      MemberLookupResultRef& result,
      size_t flags) {
    for (auto t : stem) {
      lookup(name, t, result, flags);
    }
  }

  void MemberNameLookup::lookup(
      const llvm::StringRef& name,
      const Member* stem,
      MemberLookupResultRef& result,
      size_t flags) {

    MemberLookupResult members;
    switch (stem->kind) {
      case Member::Kind::MODULE:
        static_cast<const Module*>(stem)->memberScope()->lookup(name, members, nullptr);
        break;
      case Member::Kind::TYPE: {
        auto td = static_cast<const TypeDefn*>(stem);
        if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
          lookupInherited(name, udt, result, flags);
        } else {
          td->memberScope()->lookup(name, members, nullptr);
        }
        break;
      }
      case Member::Kind::TYPE_PARAM: {
        auto tp = static_cast<const TypeParameter*>(stem);
        for (auto st : tp->subtypeConstraints()) {
          lookup(name, st, result, flags);
        }
        break;
      }
      case Member::Kind::SPECIALIZED: {
        auto sp = static_cast<const SpecializedDefn*>(stem);
        lookup(name, sp->generic(), members, flags);
        for (auto gm : members) {
          // TODO: if gm is already specialized, then flatten
          result.push_back({
            _specs.specialize(static_cast<graph::Defn*>(gm.member), sp->typeArgs()),
            gm.stem
          });
        }
        return;
      }
      case Member::Kind::VAR_DEF:
      case Member::Kind::FUNCTION_PARAM:
      case Member::Kind::ENUM_VAL:
      case Member::Kind::TUPLE_MEMBER:
      case Member::Kind::FUNCTION:
      case Member::Kind::COUNT:
        assert(false && "value defs do not have scopes");
        break;
    }

    for (auto m : members) {
      result.push_back(m);
    }
  }

  void MemberNameLookup::lookup(
      const llvm::StringRef& name,
      const Type* stem,
      MemberLookupResultRef& result,
      size_t flags) {
    if (auto udt = dyn_cast<UserDefinedType>(stem)) {
      lookup(name, udt->defn(), result, flags);
    } else if (auto pr = dyn_cast<PrimitiveType>(stem)) {
      lookup(name, pr->defn(), result, flags);
    } else {
      assert(false && "Implement lookup in type");
    }
  }

  void MemberNameLookup::lookupInherited(
      const llvm::StringRef& name,
      UserDefinedType* udt,
      MemberLookupResultRef& result,
      size_t flags) {
    if (!(flags & INHERITED_ONLY)) {
      MemberLookupResult members;
      udt->defn()->memberScope()->lookup(name, members, nullptr);

      auto isStatic = [](Member* m) -> bool {
        auto defn = dyn_cast<Defn>(m);
        return defn && defn->isStatic();
      };

      if (members.size() > 0) {
        for (auto m : members) {
          if (isStatic(m.member) ? (flags & STATIC_MEMBERS) : (flags & INSTANCE_MEMBERS)) {
            result.push_back(m);
          }
        }
        return;
      }
    }
    for (auto ext : udt->defn()->extends()) {
      lookup(name, ext, result, flags & ~(INHERITED_ONLY));
    }
  }

  void MemberNameLookup::forAllNames(const llvm::ArrayRef<Member*>& stem, const NameCallback& nameFn) {
    for (auto m : stem) {
      forAllNames(m, nameFn);
    }
  }

  void MemberNameLookup::forAllNames(Member* stem, const NameCallback& nameFn) {
    std::vector<Member*> members;
    switch (stem->kind) {
      case Member::Kind::MODULE:
        static_cast<Module*>(stem)->memberScope()->forAllNames(nameFn);
        break;
      case Member::Kind::TYPE: {
        auto td = static_cast<TypeDefn*>(stem);
        if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
          udt->defn()->memberScope()->forAllNames(nameFn);
          for (auto ext : udt->defn()->extends()) {
            forAllNames(ext, nameFn);
          }
        } else {
          td->memberScope()->forAllNames(nameFn);
        }
        break;
      }
      case Member::Kind::TYPE_PARAM: {
        auto tp = static_cast<TypeParameter*>(stem);
        for (auto st : tp->subtypeConstraints()) {
          if (auto comp = dyn_cast<UserDefinedType>(st)) {
            forAllNames(comp->defn(), nameFn);
          }
        }
        break;
      }
      case Member::Kind::SPECIALIZED:
        assert(false && "implement.");
        break;
      case Member::Kind::VAR_DEF:
      case Member::Kind::FUNCTION_PARAM:
      case Member::Kind::ENUM_VAL:
      case Member::Kind::TUPLE_MEMBER:
      case Member::Kind::FUNCTION:
      case Member::Kind::COUNT:
        break;
    }
  }

  void MemberNameLookup::forAllNames(Type* stem, const NameCallback& nameFn) {
    if (auto comp = dyn_cast<UserDefinedType>(stem)) {
      forAllNames(comp->defn(), nameFn);
    } else if (auto pr = dyn_cast<PrimitiveType>(stem)) {
      forAllNames(pr->defn(), nameFn);
    } else {
      assert(false && "Implement forAllNames in type");
    }
  }
}
