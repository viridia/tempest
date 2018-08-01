#include "tempest/sema/names/namelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/package.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

#include <cassert>
#include <llvm/Support/Casting.h>

namespace tempest::sema::names {
  using tempest::sema::graph::Module;
  // using tempest::sema::graph::Package;
  using tempest::sema::graph::PrimitiveType;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::TypeParameter;
  using tempest::sema::graph::UserDefinedType;
  using llvm::dyn_cast;
  using llvm::isa;

  void NameLookup::lookup(
      const llvm::StringRef& name,
      const llvm::ArrayRef<Member*>& stem,
      bool fromStatic,
      NameLookupResultRef& result) {
    for (auto m : stem) {
      lookup(name, m, fromStatic, result);
    }
  }

  void NameLookup::lookup(
      const llvm::StringRef& name,
      const llvm::ArrayRef<Type*>& stem,
      bool fromStatic,
      NameLookupResultRef& result) {
    for (auto t : stem) {
      lookup(name, t, fromStatic, result);
    }
  }

  void NameLookup::lookup(
      const llvm::StringRef& name,
      Member* stem,
      bool fromStatic,
      NameLookupResultRef& result) {

    std::vector<Member*> members;
    switch (stem->kind) {
      // case Member::Kind::PACKAGE:
      //   static_cast<Package*>(stem)->memberScope()->lookupName(name, members);
      //   break;
      case Member::Kind::MODULE:
        static_cast<Module*>(stem)->memberScope()->lookupName(name, members);
        break;
      case Member::Kind::TYPE: {
        auto td = static_cast<TypeDefn*>(stem);
        if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
          lookupInherited(name, udt, fromStatic, result);
        } else {
          td->memberScope()->lookupName(name, members);
        }
        break;
      }
      case Member::Kind::TYPE_PARAM: {
        auto tp = static_cast<TypeParameter*>(stem);
        for (auto st : tp->subtypeConstraints()) {
          lookup(name, st, fromStatic, result);
        }
        break;
      }
      case Member::Kind::SPECIALIZED:
        assert(false && "implement.");
        break;
      case Member::Kind::CONST_DEF:
      case Member::Kind::LET_DEF:
      case Member::Kind::FUNCTION_PARAM:
      case Member::Kind::ENUM_VAL:
      case Member::Kind::TUPLE_MEMBER:
      case Member::Kind::FUNCTION:
      case Member::Kind::COUNT:
        assert(false && "value defs do not have scopes");
        break;
    }

    for (auto m : members) {
      result.insert(m);
    }
  }

  void NameLookup::lookup(
      const llvm::StringRef& name,
      Type* stem,
      bool fromStatic,
      NameLookupResultRef& result) {
    if (auto udt = dyn_cast<UserDefinedType>(stem)) {
      lookup(name, udt->defn(), fromStatic, result);
    } else if (auto pr = dyn_cast<PrimitiveType>(stem)) {
      lookup(name, pr->defn(), fromStatic, result);
    } else {
      assert(false && "Implement lookup in type");
    }
  }

  void NameLookup::lookupInherited(
      const llvm::StringRef& name,
      UserDefinedType* udt,
      bool fromStatic,
      NameLookupResultRef& result) {
    std::vector<Member*> members;
    udt->defn()->memberScope()->lookupName(name, members);
    if (members.size() > 0) {
      for (auto m : members) {
        result.insert(m);
      }
      return;
    }
    for (auto ext : udt->extends()) {
      lookup(name, ext, fromStatic, result);
    }
  }

  void NameLookup::forAllNames(const llvm::ArrayRef<Member*>& stem, const NameCallback& nameFn) {
    for (auto m : stem) {
      forAllNames(m, nameFn);
    }
  }

  void NameLookup::forAllNames(Member* stem, const NameCallback& nameFn) {
    std::vector<Member*> members;
    switch (stem->kind) {
      // case Member::Kind::PACKAGE:
      //   static_cast<Package*>(stem)->memberScope()->forAllNames(nameFn);
      //   break;
      case Member::Kind::MODULE:
        static_cast<Module*>(stem)->memberScope()->forAllNames(nameFn);
        break;
      case Member::Kind::TYPE: {
        auto td = static_cast<TypeDefn*>(stem);
        if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
          udt->defn()->memberScope()->forAllNames(nameFn);
          for (auto ext : udt->extends()) {
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
      case Member::Kind::CONST_DEF:
      case Member::Kind::LET_DEF:
      case Member::Kind::FUNCTION_PARAM:
      case Member::Kind::ENUM_VAL:
      case Member::Kind::TUPLE_MEMBER:
      case Member::Kind::FUNCTION:
      case Member::Kind::COUNT:
        break;
    }
  }

  void NameLookup::forAllNames(Type* stem, const NameCallback& nameFn) {
    if (auto comp = dyn_cast<UserDefinedType>(stem)) {
      forAllNames(comp->defn(), nameFn);
    } else if (auto pr = dyn_cast<PrimitiveType>(stem)) {
      forAllNames(pr->defn(), nameFn);
    } else {
      assert(false && "Implement forAllNames in type");
    }
  }
}
