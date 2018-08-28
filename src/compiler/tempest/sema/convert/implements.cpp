#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/convert/applyspec.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "tempest/sema/names/membernamelookup.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;
  using namespace tempest::sema::pass;
  using namespace llvm;
  using tempest::error::diag;

  bool implementsMembers(
      const TypeDefn* dst, ConversionEnv& dstEnv,
      const TypeDefn* src, ConversionEnv& srcEnv) {
    assert(dst->type()->kind == Type::Kind::INTERFACE || dst->type()->kind == Type::Kind::TRAIT);
    // It must also implement all the members of the base interface or trait
    for (auto base : dst->extends()) {
      if (base->kind == Defn::Kind::SPECIALIZED) {
        auto sp = static_cast<const SpecializedDefn*>(base);
        ApplySpecialization apply(dstEnv.args);
        ConversionEnv newEnv;
        newEnv.params = sp->typeParams();
        for (auto param : newEnv.params) {
          auto typeArg = sp->typeArgs()[param->index()];
          newEnv.args.push_back(apply.transform(typeArg));
        }
        if (auto baseDefn = dyn_cast<TypeDefn>(sp->generic())) {
          if (!implementsMembers(baseDefn, newEnv, src, srcEnv)) {
            return false;
          }
        } else {
          assert(false && "Invalid base type");
        }
      } else if (auto baseDefn = dyn_cast<TypeDefn>(base)) {
        if (!implementsMembers(baseDefn, dstEnv, src, srcEnv)) {
          return false;
        }
      } else {
        assert(false && "Invalid base type");
      }
    }

    for (auto member : dst->members()) {
      if (member->kind == Member::Kind::FUNCTION ||
          member->kind == Member::Kind::LET_DEF ||
          member->kind == Member::Kind::CONST_DEF) {
        MemberNameLookup lookup;
        NameLookupResult lookupResult;
        lookup.lookup(member->name(), src, false, lookupResult);
        if (lookupResult.empty()) {
          return false;
        }

        if (auto vdef = dyn_cast<ValueDefn>(member)) {
          assert(false && "Implement interface field");
          // if (vdef->type() == nullptr) {
          //   assert(false && "");
          // }

        } else {
          auto fdef = static_cast<FunctionDefn*>(member);
          if (fdef->type() == nullptr) {
            ResolveTypesPass::resolve(fdef);
          }
          assert(fdef->type()->returnType != nullptr);
          FunctionDefn* target = nullptr;
          for (auto srcMember : lookupResult) {
            if (auto srcFunc = dyn_cast<FunctionDefn>(srcMember)) {
              if (!srcFunc->isStatic()) {
                // TODO: We might want to be more precise here than just 'assignable'.
                // It really means can we correctly call srcFunc through fdef's type signature.
                auto cr = isAssignable(fdef->type(), 0, dstEnv, srcFunc->type(), 0, srcEnv);
                if (cr.rank >= ConversionRank::EXACT) {
                  target = srcFunc;
                  break;
                }
              }
            }
          }
          if (!target) {
            return false;
          }
        }
      }
    }

    return true;
  }
}
