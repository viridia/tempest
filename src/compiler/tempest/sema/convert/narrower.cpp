#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::infer;
  using namespace llvm;
  using tempest::error::diag;
  using tempest::sema::transform::ApplySpecialization;

  bool isEqualOrNarrower(const Type* l, const Type* r) {
    Env emptyEnv;
    return isEqualOrNarrower(l, emptyEnv, r, emptyEnv);
  }

  bool isEqualOrNarrower(
      const Type* l, Env& lEnv,
      const Type* r, Env& rEnv) {

    // Types that are the same are equally specific regardless of type arguments.
    // So Foo[i32] is just as narrow as Foo[i64]
    // I think...
    if (l == r) {
      return true;
    }

    // if isinstance(lt, graph.TypeAlias):
    //   lt = lt.getValue()

    // if isinstance(rt, graph.TypeAlias):
    //   rt = rt.getValue()

    if (l->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(l);
      ApplySpecialization apply(lEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return isEqualOrNarrower(genType, newEnv, r, rEnv);
    }

    if (r->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(r);
      ApplySpecialization apply(rEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return isEqualOrNarrower(l, lEnv, genType, newEnv);
    }

    if (l->kind == Type::Kind::INFERRED) {
      auto lit = static_cast<const InferredType*>(l);
      for (auto& constraint : lit->constraints) {
        if (lit->isViable(constraint) && isEqualOrNarrower(constraint.value, lEnv, r, rEnv)) {
          return true;
        }
      }
      return false;
    }

    if (r->kind == Type::Kind::INFERRED) {
      auto rit = static_cast<const InferredType*>(r);
      for (auto& constraint : rit->constraints) {
        if (rit->isViable(constraint) && isEqualOrNarrower(l, lEnv, constraint.value, rEnv)) {
          return true;
        }
      }
      return false;
    }

    // if (l->kind == Type::Kind::MODIFIED) {
    //   auto lMod = static_cast<const ModifiedType*>(l);
    //   if (r->kind == Type::Kind::MODIFIED) {
    //     auto rMod = static_cast<const ModifiedType*>(r);
    //     if (!lMod->isImmutable && rMod->isImmutable) {
    //       return false;
    //     }
    //   } else {
    //   }
    // }

    // if (r->kind == Type::Kind::MODIFIED) {
    //   auto rMod = static_cast<const ModifiedType*>(r);
    // }

//   if isinstance(lt, graph.ModifiedType):
//     if isinstance(rt, graph.ModifiedType):
//       spec = compareSpecificityExt(lt.getBase(), isConst(lt), lEnv, rt.getBase(), isConst(rt), rEnv)
//       if spec != RelativeSpecificity.EQUAL:
//         return spec
//       if isVariadic(lt) and not isVariadic(rt):
//         return RelativeSpecificity.LESS;
//       elif not isVariadic(lt) and isVariadic(rt):
//         return RelativeSpecificity.MORE;
//       else:
//         return spec
//     else:
//       spec = compareSpecificityExt(lt.getBase(), isConst(lt), lEnv, rt, rt, rEnv)
//       if spec != RelativeSpecificity.EQUAL:
//         return spec
//       if isVariadic(lt):
//         return RelativeSpecificity.LESS;
//       else:
//         return spec
//   elif isinstance(rt, graph.ModifiedType):
//     spec = compareSpecificityExt(lt, lt, lEnv, rt.getBase(), isConst(rt), rEnv)
//     if spec != RelativeSpecificity.EQUAL:
//       return spec
//     if isVariadic(rt):
//       return RelativeSpecificity.MORE;
//     else:
//       return spec

//   if isinstance(lt, graph.TypeVar):
//     if isinstance(rt, graph.TypeVar):
//       assert False, ("Implement", len(lt.constraints), type(rt))
//     elif rt is primitivetypes.ANY:
//       return RelativeSpecificity.MORE;
//     else:
//       # Only the left side is a type variable. To say that left's upper bounds are more specific
//       # than the right-hand type, requires that right is a supertype of some member of left.
//       # (They can't ever be equal.)

//       # Temporarily just say that left is less specific because it's a tvar
//       return RelativeSpecificity.LESS
//   elif isinstance(rt, graph.TypeVar):
//     # If they are equal, but right is a type variable and left is not, then left is more specific
//     if isEqualExt(lt, lConst, lEnv, rt, rConst, rEnv):
//       if isinstance(lt, graph.TypeVar):
//         return RelativeSpecificity.EQUAL;
//       return RelativeSpecificity.MORE;
//     if rt in rEnv:
//       debug.fail("Implement", lt, lEnv, rt, rEnv)

//     if lt is primitivetypes.ANY:
//       return RelativeSpecificity.LESS;

//     return RelativeSpecificity.DISJOINT;

// #     // Only the right side is a type variable. To say that left is more specific than the
// #     // upper bounds of right would be true only if left is a subtype of *every* type in right.
// #     const TypeVariable * rtv = rhs.as<TypeAssignment>()->target();
// #     const QualifiedTypeList & ru = rtv->upperBounds();
// #     for (QualifiedTypeList::const_iterator ri = ru.begin(); ri != ru.end(); ++ri) {
// #       if (!isSubtype(lhs, *ri)) {
// #         // A type variable is less specific than a concrete type.
// #         return NOT_MORE_SPECIFIC;
// #       }
// #     }

//     return RelativeSpecificity.MORE;

    if (l->kind == Type::Kind::UNION) {
      auto lUnion = static_cast<const UnionType*>(l);
      // So say that a union is narrower than some type means that all members of the union
      // are narrower than that type.
      for (auto memberType : lUnion->members) {
        if (!isEqualOrNarrower(memberType, lEnv, r, rEnv)) {
          return false;
        }
      }
      return true;
    }

    if (r->kind == Type::Kind::UNION) {
      auto rUnion = static_cast<const UnionType*>(r);
      // So say that a type if narrower than a union means that it is narrower than any type
      // in the union.
      for (auto memberType : rUnion->members) {
        if (isEqualOrNarrower(l, lEnv, memberType, rEnv)) {
          return true;
        }
      }
      return false;
    }

    if (l->kind == Type::Kind::VOID) {
      return r->kind == Type::Kind::VOID;
    }

    if (l->kind == Type::Kind::BOOLEAN) {
      return r->kind == Type::Kind::BOOLEAN;
    }

    if (l->kind == Type::Kind::INTEGER) {
      auto lInt = static_cast<const IntegerType*>(l);
      if (r->kind == Type::Kind::INTEGER) {
        auto rInt = static_cast<const IntegerType*>(r);
        if (lInt->isUnsigned() == rInt->isUnsigned()) {
          return lInt->bits() <= rInt->bits();
        } else {
          return false;
        }
      }
      return false;
    }

    if (l->kind == Type::Kind::FLOAT) {
      if (r->kind == Type::Kind::FLOAT) {
        auto lFloat = static_cast<const FloatType*>(l);
        auto rFloat = static_cast<const FloatType*>(r);
        return lFloat->bits() <= rFloat->bits();
      }
      return false;
    }

    if (l->kind == Type::Kind::CLASS) {
        auto lClass = static_cast<const UserDefinedType*>(l);
      if (r->kind == Type::Kind::CLASS) {
        // A class can be narrower than a trait or interface, but not vice-versa.
        auto rClass = static_cast<const UserDefinedType*>(r);
        return isSubtype(lClass->defn(), lEnv, rClass->defn(), rEnv);
      } else if (r->kind == Type::Kind::INTERFACE || r->kind == Type::Kind::TRAIT) {
        // A class can be narrower than a trait or interface, but not vice-versa.
        auto rClass = static_cast<const UserDefinedType*>(r);
        return isSubtype(lClass->defn(), lEnv, rClass->defn(), rEnv)
            || implementsMembers(lClass->defn(), lEnv, rClass->defn(), rEnv);
      }
      return false;
    }

    if (l->kind == Type::Kind::STRUCT) {
      return false;
    }

    if (l->kind == Type::Kind::INTERFACE && r->kind == Type::Kind::INTERFACE) {
      auto lClass = static_cast<const UserDefinedType*>(l);
      auto rClass = static_cast<const UserDefinedType*>(r);
      return isSubtype(lClass->defn(), lEnv, rClass->defn(), rEnv)
          || implementsMembers(lClass->defn(), lEnv, rClass->defn(), rEnv);
    }

    if (l->kind == Type::Kind::TRAIT && r->kind == Type::Kind::TRAIT) {
      auto lClass = static_cast<const UserDefinedType*>(l);
      auto rClass = static_cast<const UserDefinedType*>(r);
      return isSubtype(lClass->defn(), lEnv, rClass->defn(), rEnv)
          || implementsMembers(lClass->defn(), lEnv, rClass->defn(), rEnv);
    }

    if (l->kind == Type::Kind::ENUM) {
      // Should have already succeeded at this point.
      if (r->kind == Type::Kind::INTEGER) {
        auto lEnum = static_cast<const UserDefinedType*>(l);
        auto lInt = cast<IntegerType>(cast<TypeDefn>(lEnum->defn()->extends()[0])->type());
        auto rInt = static_cast<const IntegerType*>(r);
        return isEqualOrNarrower(lInt, lEnv, rInt, rEnv);
      }
      return false;
    }

    if (l->kind == Type::Kind::TUPLE) {
      if (r->kind == Type::Kind::TUPLE) {
        auto lTuple = static_cast<const TupleType*>(l);
        auto rTuple = static_cast<const TupleType*>(r);
        // For tuples, every member must be equal or narrower than the corresponding member
        if (lTuple->members.size() == rTuple->members.size()) {
          for (size_t i = 0; i < lTuple->members.size(); ++i) {
            if (!isEqualOrNarrower(lTuple->members[i], lEnv, rTuple->members[i], rEnv)) {
              return false;
            }
          }
          return true;
        }
      }
      return false;
    }

    if (l->kind == Type::Kind::FUNCTION) {
      if (r->kind == Type::Kind::FUNCTION) {
        auto lFunc = static_cast<const FunctionType*>(l);
        auto rFunc = static_cast<const FunctionType*>(r);
        // A function with const self is narrower.
        if (!lFunc->constSelf && rFunc->constSelf) {
          return false;
        }
        assert(lFunc->returnType);
        assert(rFunc->returnType);
        if (!isEqualOrNarrower(lFunc->returnType, lEnv, rFunc->returnType, rEnv)) {
          return false;
        }
        // For now let's say that functions with different numbers of params are disjoint.
        if (lFunc->paramTypes.size() != rFunc->paramTypes.size()) {
          return false;
        }
        if (lFunc->isVariadic != rFunc->isVariadic) {
          return false;
        }
        // TODO: I'm not sure this is right - are parameters of function type covariant?
        for (size_t i = 0; i < rFunc->paramTypes.size(); i += 1) {
          if (!isEqualOrNarrower(lFunc->paramTypes[i], lEnv, rFunc->paramTypes[i], rEnv)) {
            return false;
          }
        }
        return true;
      }
      return false;
    }

    diag.fatal() << "isEqualOrNarrower not handled: " << l << " : " << r;
    assert(false && "isEqualOrNarrower not handled");
    return false;
  }
}
