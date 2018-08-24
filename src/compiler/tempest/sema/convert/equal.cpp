#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/convert/applyspec.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace llvm;
  using tempest::error::diag;

  bool isEqual(const Type* dst, const Type* src) {
    ConversionEnv srcEnv;
    ConversionEnv dstEnv;
    return isEqual(dst, 0, dstEnv, src, 0, srcEnv);
  }

  bool isEqual(
    const Type* dst, uint32_t dstMods, ConversionEnv& dstEnv,
    const Type* src, uint32_t srcMods, ConversionEnv& srcEnv) {

    if (src == dst && srcMods == dstMods) {
      if (isa<PrimitiveType>(src) ||
          src->kind == Type::Kind::TYPE_VAR ||
          src->kind == Type::Kind::ALIAS ||
          src->kind == Type::Kind::SINGLETON) {
        return true;
      } else {
        if (dstEnv.args.size() == srcEnv.args.size()) {
          for (size_t i = 0; i < dstEnv.args.size(); i++) {
            if (!isEqual(dstEnv.args[i], srcEnv.args[i])) {
              return false;
            }
          }
          return true;
        }
      }
    }

    if (dst->kind == Type::Kind::ALIAS) {
      return isEqual(
          static_cast<const UserDefinedType*>(dst)->defn()->aliasTarget(), dstMods, dstEnv,
          src, srcMods, srcEnv);
    }

    if (src->kind == Type::Kind::ALIAS) {
      return isEqual(
          dst, dstMods, dstEnv,
          static_cast<const UserDefinedType*>(src)->defn()->aliasTarget(), srcMods, srcEnv);
    }

    if (src->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(src);
      ApplySpecialization apply(srcEnv.args);
      ConversionEnv newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      return isEqual(src, srcMods, newEnv, dst, dstMods, dstEnv);
    }

    if (dst->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(dst);
      ApplySpecialization apply(dstEnv.args);
      ConversionEnv newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      return isEqual(src, srcMods, srcEnv, dst, dstMods, newEnv);
    }

//   if isinstance(lt, graph.ModifiedType):
//     if isConst(lt):
//       lConst = True
//     with debug.indented():
//       debug.trace('check modified/?')
//       return isEqualExt(lt.getBase(), lConst, lEnv, rt, rConst, rEnv)

//   if isinstance(rt, graph.ModifiedType):
//     if isConst(rt):
//       rConst = True
//     with debug.indented():
//       debug.trace('check ?/modified')
//       return isEqualExt(lt, lConst, lEnv, rt.getBase(), rConst, rEnv)

//   if isinstance(lt, graph.InstantiateType):
//     with debug.indented():
//       debug.trace('check instantiated/?', lt.getEnv())
//       return isEqualExt(lt.getBase(), lConst, lEnv.compose(lt.getEnv()), rt, rConst, rEnv)
// #       return isEqualExt(lt.getBase(), lConst, (lt.getEnv(), lEnv), rt, rConst, rEnv)

//   if isinstance(rt, graph.InstantiateType):
//     with debug.indented():
//       debug.trace('check ?/instantiated/?', rt.getEnv())
//       return isEqualExt(lt, lConst, lEnv, rt.getBase(), rConst, rEnv.compose(rt.getEnv()))

//   if isinstance(rt, ContingentType):
//     for mty, when in rt.types:
//       if when.isViable():
//         if isEqualExt(lt, lConst, lEnv, mty, rConst, rEnv):
//           return True
//     return False

    if (dst->kind == Type::Kind::VOID) {
      return false;
    }

    if (dst->kind == Type::Kind::BOOLEAN) {
      return false;
    }

    if (dst->kind == Type::Kind::INTEGER) {
      auto dstInt = static_cast<const IntegerType*>(dst);
      if (src->kind == Type::Kind::INTEGER) {
        auto srcInt = static_cast<const IntegerType*>(src);
        return dstInt->isUnsigned() == srcInt->isUnsigned() && srcInt->bits() == dstInt->bits();
      }
      return false;
    }

    if (dst->kind == Type::Kind::FLOAT) {
      if (src->kind == Type::Kind::FLOAT) {
        auto dstFloat = static_cast<const FloatType*>(dst);
        auto srcFloat = static_cast<const FloatType*>(src);
        return srcFloat->bits() == dstFloat->bits();
      }
      return false;
    }

    if (dst->kind == Type::Kind::ENUM) {
      return false;
    }


//   elif isinstance(lt, graph.Composite):
//     if isinstance(rt, graph.Composite):
//       with debug.indented():
//         debug.trace('check composite')
//         return isEqualComposite(lt, lConst, lEnv, rt, rConst, rEnv)
//     elif not isinstance(rt, graph.TypeVar):
//       debug.trace('-- different (composite/?)', type(rt))
//       return False;

//   elif isinstance(lt, graph.FunctionType):
//     if isinstance(rt, graph.FunctionType):
//       with debug.indented():
//         debug.trace('check function')
//         return isEqualFunction(lt, lConst, lEnv, rt, rConst, rEnv);
//     elif not isinstance(rt, graph.TypeVar):
//       debug.trace('-- different (function/?)')
//       return False

//   elif isinstance(lt, graph.TupleType):
//     if isinstance(rt, graph.TupleType):
//       with debug.indented():
//         debug.trace('check tuple')
//         return isEqualTuple(lt, lConst, lEnv, rt, rConst, rEnv);
//     elif not isinstance(rt, graph.TypeVar):
//       debug.trace('-- different (tuple/?)')
//       return False

//   elif isinstance(lt, graph.UnionType):
//     if isinstance(rt, graph.UnionType):
//       return isEqualUnion(lt, lConst, lEnv, rt, rConst, rEnv);
//     elif not isinstance(rt, graph.TypeVar):
//       debug.trace('-- different (union/?)')
//       return False

//   elif isinstance(lt, graph.TypeVar):
//     if isinstance(rt, graph.TypeVar):
// #       if isinstance(lt, renamer.InferredTypeVar) != isinstance(rt, renamer.InferredTypeVar):
// #         debug.write('Matching an inferred vs non-inferred type var:', lt, rt)
// #         assert False
//       with debug.indented():
//         if lt.getParam() == rt.getParam() and not lEnv and not rEnv:
//           if not lt.hasIndex() and not rt.hasIndex():
//             debug.trace('-- same param, no index')
//             return True
//           elif lt.hasIndex() and rt.hasIndex() and lt.getIndex() == rt.getIndex():
//             debug.trace('-- same param, same index')
//             return True
//     if lt in lEnv:
//       with debug.indented():
//         debug.trace('mapped', lt, 'to', lEnv[lt])
//         if isEqualExt(lEnv[lt], lConst, environ.EMPTY, rt, rConst, rEnv):
//           return True
//     else:
//       debug.trace(lt, 'not in', lEnv)
//     if isinstance(lt, renamer.InferredTypeVar):
//       with debug.indented():
//         for ct in lt.constraints:
//           assert ct.typeVar is lt
//           if not ct.isViable():
//             continue
//           if isinstance(ct, EquivalenceConstraint):
//             # If it's an equivalence constraint, then it's commutative
//             debug.trace('checking constraint', lt, '==', (ct.value, lEnv), 'with', (rt, rEnv))
//             if isEqualExt(ct.value, False, lEnv, rt, rConst, rEnv):
//               return True
//           elif isinstance(ct, SubtypeConstraint):
//             # if lt <: X, then lt can be equal to rt if rt is also <: X
//             debug.trace('checking constraint', lt, '<:', (ct.value, lEnv), 'with', (rt, rEnv))
//             if isSubtypeExt(rt, rConst, rEnv, ct.value, False, lEnv):
//               debug.trace('-- success')
//               return True
//           elif isinstance(ct, SupertypeConstraint):
//             # if X <: lt, then lt can be equal to rt if X <: rt
//             debug.trace('checking constraint', lt, ':>', (ct.value, lEnv), 'with', (rt, rEnv))
//             if isSubtypeExt(ct.value, False, lEnv, rt, rConst, rEnv):
//               debug.trace('-- success')
//               return True
//     if not isinstance(rt, graph.TypeVar):
//       debug.trace('-- failed (rt not typevar)')
//       return False

//   if isinstance(rt, graph.TypeVar):
//     if rt in rEnv:
//       if isEqualExt(lt, lConst, lEnv, rEnv[rt], rConst, environ.EMPTY):
//         return True
//     if isinstance(rt, renamer.InferredTypeVar):
//       for ct in rt.constraints:
//         assert ct.typeVar is rt
//         if not ct.isViable():
//           continue
//         if isinstance(ct, EquivalenceConstraint):
//           # If it's an equivalence constraint, then it's commutative
//           if isEqualExt(lt, False, lEnv, ct.value, rConst, rEnv):
//             return True
//         elif isinstance(ct, SubtypeConstraint):
//           # if rt <: X, then lt can be equal to rt if lt is also <: X
//           if isSubtypeExt(lt, False, lEnv, ct.value, False, rEnv):
//             return True
//         elif isinstance(ct, SupertypeConstraint):
//           # if X <: rt, then lt can be equal to rt if X <: lt
//           if isSubtypeExt(ct.value, False, rEnv, lt, False, lEnv):
//             return True
//     return False

//   if isinstance(lt, ContingentType):
//     for mty, when in lt.types:
//       if when.isViable():
//         if isEqualExt(mty, lConst, lEnv, rt, rConst, rEnv):
//           return True
//     return False

// #   if isinstance(lt, ContingentType):
// #     for (aty, when) in lt.types:
// #       if all(w.isViable() for w in when):
// #         if isEqualEnv(aty, lConst, lEnv, rt, rConst, rEnv):
// #           return True
// #     return False
// #
// #   if isinstance(rt, ContingentType):
// #     for (aty, when) in rt.types:
// #       if all(w.isViable() for w in when):
// #         if isEqualEnv(lt, lConst, lEnv, aty, rConst, rEnv):
// #           return True
// #     return False

//   else:
//     debug.write('isEqual not handled:', lt, lEnv, '==', rt, rEnv)
//     return False

//   debug.write('isEqual not handled:', lt, lEnv, '==', rt, rEnv)

    return false;
  }
}
