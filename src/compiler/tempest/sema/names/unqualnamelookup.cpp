#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

#include <cassert>
#include <llvm/Support/Casting.h>

namespace tempest::sema::names {
  using tempest::sema::graph::Module;
  using tempest::sema::graph::PrimitiveType;
  using tempest::sema::graph::NameLookupResult;
  using llvm::dyn_cast;

  void ModuleScope::lookup(const llvm::StringRef& name, NameLookupResultRef& result) {
    module->memberScope()->lookupName(name, result);
    if (result.empty() && prev) {
      prev->lookup(name, result);
    }
  }

  void ModuleScope::forEach(const NameCallback& nameFn) {
    module->memberScope()->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }

  void TypeDefnScope::lookup(const llvm::StringRef& name, NameLookupResultRef& result) {
    auto resultSize = result.size();
    typeDefn->memberScope()->lookupName(name, result);
    if (result.size() > resultSize) {
      // If we found a member with that name, return
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
          typeDefn->typeParamScope()->lookupName(name, result);
          if (result.size() > resultSize) {
            // If we found a type parameter with that name, return it.
            return;
          }
          assert(false && "Implement");
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

  void LocalScope::lookup(const llvm::StringRef& name, NameLookupResultRef& result) {
  }

  void LocalScope::forEach(const NameCallback& nameFn) {
    // typeDefn->memberScope()->forAllNames(nameFn);
    if (prev) {
      prev->forEach(nameFn);
    }
  }
}
