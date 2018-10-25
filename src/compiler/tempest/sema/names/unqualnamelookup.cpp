#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/specstore.hpp"

#include <cassert>

namespace tempest::sema::names {
  using tempest::intrinsic::IntrinsicDefns;
  using tempest::sema::graph::Module;
  using tempest::sema::graph::PrimitiveType;
  using tempest::sema::graph::MemberLookupResult;
  using tempest::sema::graph::SpecializedDefn;
  using llvm::dyn_cast;

  void ModuleScope::lookup(const llvm::StringRef& name, MemberLookupResultRef& result) {
    module->memberScope()->lookup(name, result, nullptr);
    // TODO: Core module.
    if (result.empty()) {
      IntrinsicDefns::get()->builtinScope->lookup(name, result, nullptr);
    }
    if (result.empty() && prev) {
      prev->lookup(name, result);
    }
  }

  void ModuleScope::forEach(const NameCallback& nameFn) {
    module->memberScope()->forAllNames(nameFn);
    // TODO: Core module.
    IntrinsicDefns::get()->builtinScope->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  void TypeParamScope::lookup(const llvm::StringRef& name, MemberLookupResultRef& result) {
    auto resultSize = result.size();
    generic->typeParamScope()->lookup(name, result, nullptr);
    if (result.size() <= resultSize && prev) {
      prev->lookup(name, result);
    }
  }

  void TypeParamScope::forEach(const NameCallback& nameFn) {
    generic->typeParamScope()->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  void TypeDefnScope::lookup(const llvm::StringRef& name, MemberLookupResultRef& result) {
    MemberLookupResult memberResults;
    typeDefn->memberScope()->lookup(name, memberResults, typeDefn->implicitSelf());
    auto isInstanceMember = [](Member* m) -> bool {
      if (auto d = dyn_cast<graph::Defn>(m)) {
        return d->isMember();
      }
      return false;
    };

    if (memberResults.size() > 0) {
      // If we found any members with that name, return, don't check inherited scopes.
      for (auto& m : memberResults) {
        result.push_back({ m.member, isInstanceMember(m.member) ? m.stem : nullptr });
      }
      return;
    }

    if (auto udt = dyn_cast<UserDefinedType>(typeDefn->type())) {
      switch (udt->kind) {
        case Type::Kind::CLASS:
        case Type::Kind::STRUCT:
        case Type::Kind::INTERFACE:
        case Type::Kind::TRAIT:
        case Type::Kind::EXTENSION: {
          auto resultSize = result.size();
          typeDefn->typeParamScope()->lookup(name, result, nullptr);
          if (result.size() > resultSize) {
            // If we found a type parameter with that name, return it.
            return;
          }
          for (auto base : typeDefn->extends()) {
            if (auto sp = dyn_cast<SpecializedDefn>(base)) {
              auto generic = cast<TypeDefn>(sp->generic());
              generic->memberScope()->lookup(name, memberResults, typeDefn->implicitSelf());
              // TODO: if gm is already specialized, then flatten
              for (auto& m : memberResults) {
                result.push_back({
                  spec.specialize(static_cast<graph::Defn*>(m.member), sp->typeArgs()),
                  isInstanceMember(m.member) ? m.stem : nullptr });
              }
            } else {
              auto baseDefn = cast<TypeDefn>(base);
              baseDefn->memberScope()->lookup(name, memberResults, typeDefn->implicitSelf());
              for (auto& m : memberResults) {
                result.push_back({ m.member, isInstanceMember(m.member) ? m.stem : nullptr });
              }
            }
          }
          // assert(false && "Implement");
          break;
        }

        case Type::Kind::ENUM: {
          break;
        }

        default:
          break;
      }
    }

    if (result.empty() && prev) {
      prev->lookup(name, result);
    }
  }

  void TypeDefnScope::forEach(const NameCallback& nameFn) {
    typeDefn->memberScope()->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  void FunctionScope::lookup(const llvm::StringRef& name, MemberLookupResultRef& result) {
    auto resultSize = result.size();
    funcDefn->paramScope()->lookup(name, result, nullptr);

    if (result.size() <= resultSize && !funcDefn->typeParams().empty()) {
      funcDefn->typeParamScope()->lookup(name, result, nullptr);
    }

    if (result.size() <= resultSize && prev) {
      prev->lookup(name, result);
    }
  }

  void FunctionScope::forEach(const NameCallback& nameFn) {
    funcDefn->paramScope()->forAllNames(nameFn);
    funcDefn->typeParamScope()->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  void LocalScope::lookup(const llvm::StringRef& name, MemberLookupResultRef& result) {
    // For local scopes, we only return the most recent definition of a name.
    MemberLookupResult names;
    _symbols.lookup(name, names, nullptr);
    if (names.size() > 0) {
      result.push_back(names.back());
    } else {
      prev->lookup(name, result);
    }
  }

  void LocalScope::forEach(const NameCallback& nameFn) {
    _symbols.forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  bool LocalScope::addMember(Member* member) {
    _symbols.addMember(member);
    return true;
  }
}
