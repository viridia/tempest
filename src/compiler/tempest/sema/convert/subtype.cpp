#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/convert/applyspec.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace llvm;
  using tempest::error::diag;

  bool isSubtype(
      const TypeDefn* dst, ConversionEnv& dstEnv,
      const TypeDefn* src, ConversionEnv& srcEnv) {
    if (dst == src) {
      if (dstEnv.args.size() != srcEnv.args.size()) {
        return false;
      }
      // Make sure type arguments are the same
      for (auto param : dst->allTypeParams()) {
        if (!isEqual(dstEnv.args[param->index()], srcEnv.args[param->index()])) {
          return false;
        }
      }
      return true;
    }

    MemberArray bases;
    if (dst->type()->kind == src->type()->kind) {
      bases = src->extends();
    } else if (dst->type()->kind == Type::Kind::INTERFACE
        || dst->type()->kind == Type::Kind::TRAIT) {
      bases = src->implements();
    } else {
      return false;
    }

    for (auto base : bases) {
      if (base->kind == Defn::Kind::SPECIALIZED) {
        auto sp = static_cast<const SpecializedDefn*>(base);
        ApplySpecialization apply(srcEnv.args);
        ConversionEnv newEnv;
        newEnv.params = sp->typeParams();
        for (auto param : newEnv.params) {
          auto typeArg = sp->typeArgs()[param->index()];
          newEnv.args.push_back(apply.transform(typeArg));
        }
        if (auto baseDefn = dyn_cast<TypeDefn>(sp->generic())) {
          if (isSubtype(dst, dstEnv, baseDefn, newEnv)) {
            return true;
          }
        } else {
          assert(false && "Invalid base type");
        }
      } else if (auto baseDefn = dyn_cast<TypeDefn>(base)) {
        if (isSubtype(dst, dstEnv, baseDefn, srcEnv)) {
          return true;
        }
      } else {
        assert(false && "Invalid base type");
      }
    }
    return false;
  }

  // TODO: Check for structural typing.
  // If src is a class and dst is an interface or trait;
  // if src is a struct, interface, trait, alias, and dst is a trait.
  // And we probably want to cache the result.
}
