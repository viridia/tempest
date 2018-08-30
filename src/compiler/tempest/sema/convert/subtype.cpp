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
      const TypeDefn* st, ConversionEnv& stEnv,
      const TypeDefn* bt, ConversionEnv& btEnv) {
    if (bt == st) {
      if (btEnv.args.size() != stEnv.args.size()) {
        return false;
      }
      // Make sure type arguments are the same
      for (auto param : bt->allTypeParams()) {
        if (!isEqual(btEnv.args[param->index()], stEnv.args[param->index()])) {
          return false;
        }
      }
      return true;
    }

    MemberArray bases;
    if (bt->type()->kind == st->type()->kind) {
      bases = st->extends();
    } else if (bt->type()->kind == Type::Kind::INTERFACE || bt->type()->kind == Type::Kind::TRAIT) {
      bases = st->implements();
    } else {
      return false;
    }

    for (auto base : bases) {
      if (base->kind == Defn::Kind::SPECIALIZED) {
        auto sp = static_cast<const SpecializedDefn*>(base);
        ApplySpecialization apply(stEnv.args);
        ConversionEnv newEnv;
        newEnv.params = sp->typeParams();
        for (auto param : newEnv.params) {
          auto typeArg = sp->typeArgs()[param->index()];
          newEnv.args.push_back(apply.transform(typeArg));
        }
        if (auto baseDefn = dyn_cast<TypeDefn>(sp->generic())) {
          if (isSubtype(baseDefn, newEnv, bt, btEnv)) {
            return true;
          }
        } else {
          assert(false && "Invalid base type");
        }
      } else if (auto baseDefn = dyn_cast<TypeDefn>(base)) {
        if (isSubtype(baseDefn, stEnv, bt, btEnv)) {
          return true;
        }
      } else {
        assert(false && "Invalid base type");
      }
    }
    return false;
  }

  // TODO: Check for structural typing.
  // If st is a class and bt is an interface or trait;
  // if st is a struct, interface, trait, alias, and bt is a trait.
  // And we probably want to cache the result.
}
