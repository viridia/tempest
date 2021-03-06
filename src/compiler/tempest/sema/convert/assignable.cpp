#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/infer/overload.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::infer;
  using namespace llvm;
  using tempest::error::diag;
  using tempest::sema::transform::TempMapTypeVars;

  ConversionResult isAssignable(const Type* dst, const Type* src) {
    Env srcEnv;
    Env dstEnv;
    return isAssignable(dst, 0, dstEnv, src, 0, srcEnv);
  }

  ConversionResult isAssignable(
    const Type* dst, uint32_t dstMods, Env& dstEnv,
    const Type* src, uint32_t srcMods, Env& srcEnv) {

    if (src == dst && srcMods == dstMods) {
      if (isa<PrimitiveType>(src) ||
          src->kind == Type::Kind::ENUM ||
          src->kind == Type::Kind::TYPE_VAR ||
          src->kind == Type::Kind::ALIAS ||
          src->kind == Type::Kind::SINGLETON) {
        return ConversionResult(ConversionRank::IDENTICAL);
      } else {
        if (dstEnv.args.size() == srcEnv.args.size()) {
          if (isEqual(dstEnv.args, srcEnv.args)) {
            return ConversionResult(ConversionRank::IDENTICAL);
          }
        }
      }
    }

    if (dst->kind == Type::Kind::INVALID || src->kind == Type::Kind::INVALID) {
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::ALIAS) {
      // TODO: Convert identical to exact?
      dst = static_cast<const UserDefinedType*>(dst)->defn()->aliasTarget();
    }

    if (src->kind == Type::Kind::ALIAS) {
      src = static_cast<const UserDefinedType*>(src)->defn()->aliasTarget();
    }

    if (src->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(src);
      TempMapTypeVars apply(srcEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      newEnv.args = apply.transformArray(sp->spec->typeArgs());
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return isAssignable(dst, dstMods, dstEnv, genType, srcMods, newEnv);
    }

    if (dst->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(dst);
      TempMapTypeVars apply(dstEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      newEnv.args = apply.transformArray(sp->spec->typeArgs());
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return isAssignable(genType, dstMods, newEnv, src, srcMods, srcEnv);
    }

//   if isinstance(src, graph.ModifiedType):
//     with debug.indented():
//       if isConst(src):
//         srcConst = True
//       return isAssignableEnv(dst, dstConst, dstEnv, src.getBase(), srcConst, srcEnv)

    if (dst->kind == Type::Kind::INFERRED) {
      ConversionResult result;
      auto dstIt = static_cast<const InferredType*>(dst);
      for (auto& constraint : dstIt->constraints) {
        if (dstIt->isViable(constraint)) {
          result = result.better(
              isAssignable(constraint.value, dstMods, dstEnv, src, srcMods, srcEnv));
        }
      }
      return result;
    }

    if (src->kind == Type::Kind::INFERRED) {
      ConversionResult result;
      auto srcIt = static_cast<const InferredType*>(src);
      for (auto& constraint : srcIt->constraints) {
        if (srcIt->isViable(constraint)) {
          result = result.better(
              isAssignable(dst, dstMods, dstEnv, constraint.value, srcMods, srcEnv));
        }
      }
      return result;
    }

    if (dst->kind == Type::Kind::TYPE_VAR) {
      auto dstTypeVar = static_cast<const TypeVar*>(dst);
      if (size_t(dstTypeVar->index()) < dstEnv.args.size()) {
        Env newEnv;
        assert(dstEnv.has(dstTypeVar));
        return isAssignable(
            dstEnv.args[dstTypeVar->index()], dstMods, newEnv,
            src, srcMods, srcEnv);
      }
      auto dstParam = dstTypeVar->param;
      if (!dstParam->subtypeConstraints().empty()) {
        assert(false && "Implement subtype constraints");
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (src->kind == Type::Kind::TYPE_VAR) {
      auto srcTypeVar = static_cast<const TypeVar*>(src);
      if (size_t(srcTypeVar->index()) < srcEnv.args.size()) {
        Env newEnv;
        assert(srcEnv.has(srcTypeVar));
        return isAssignable(
            dst, dstMods, dstEnv,
            srcEnv.args[srcTypeVar->index()], srcMods, newEnv);
      }
      auto srcParam = srcTypeVar->param;
      if (!srcParam->subtypeConstraints().empty()) {
        assert(false && "Implement subtype constraints");
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

//   if isinstance(src, graph.TypeVar):
//     if srcEnv:
//       env, *envList = srcEnv
//       if src.getParam().getTypeVar() in env:
//         return isAssignableEnv(
//             dst, dstConst, dstEnv, env[src.getParam().getTypeVar()], srcConst, envList)
//     if isinstance(src, renamer.InferredTypeVar):
//       bestResult = ConversionResult(ConversionRank.UNSET, None, None)
//       # If the source type is an inferred type variable, then check to see if any of the
//       # viable constraints on that variable produce a type that is assignable to the destination
//       # type.
//       # Not sure this logic is correct - I think what we really want is:
//       # -- constraints which have no conditions *must* be satisfied
//       # -- constraints which fail invalidate the set of their conditions.
//       #
//       # A different way to write this function would be 'assignable if' - it returns the list
//       # of assignable relations which must be true for the types to be assignable. An empty
//       # list indicates that it is unconditionally true; A null value indivcates failure.
//       for ct in src.constraints:
//         assert ct.typeVar is src
// #         if not ct.isViable() and not isinstance(ct, SupertypeConstraint):
// #           continue
//         # Ignore supertype constraints because they don't tell us anything about assignability;
//         # If A is assignable to B, it doesn't mean that supertypes of A are assignable to B
//         # (Actually, if A is assignable to B, then subtypes of A are also assignable to B, but
//         # only for some type kinds like reference types and ints.)
//         if isinstance(ct, SupertypeConstraint):
//           continue
//         if not ct.isViable():
//           continue
//         with debug.indented():
//           debug.trace('via src constraint:', ct)
//           result = isAssignableEnv(dst, dstConst, dstEnv, ct.value, srcConst, srcEnv)
//           if result.rank == ConversionRank.IDENTICAL and not isinstance(ct, EquivalenceConstraint):
//             result = ConversionResult(ConversionRank.EXACT, result.error, result.via)
//           if result.rank > bestResult.rank:
//             bestResult = result
//             if result.rank == ConversionRank.IDENTICAL:
//               break
//       return bestResult
//     if isinstance(dst, graph.Interface) and isModelOf(src, dst):
//       return ConversionResult(ConversionRank.INEXACT, None, None)
//     if not isinstance(dst, graph.UnionType):
//       # Special case for unions - the union might have this type var in it
//       debug.trace('-- incompatible')
//       return ConversionResult(ConversionRank.ERROR, ConversionError.INCOMPATIBLE, None)

    if (src->kind == Type::Kind::UNION) {
      // If the source is a union, choose the WORST of all possible conversion results.
      // Typically this will only make sense when the destination is also a union, although
      // it could also work if the union members have a common base type.
      auto srcUnion = static_cast<const UnionType*>(src);
      ConversionResult result(ConversionRank::EXACT);
      for (auto memberType : srcUnion->members) {
        result = result.worse(isAssignable(dst, dstMods, dstEnv, memberType, srcMods, srcEnv));
        if (result.rank == ConversionRank::ERROR) {
          break;
        }
      }
      return result;
    }

    if (src->kind == Type::Kind::CONTINGENT) {
      auto srcCont = static_cast<const ContingentType*>(src);
      ConversionResult result(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
      for (auto& entry : srcCont->entries) {
        if (entry.when->isViable()) {
          result = result.better(isAssignable(dst, dstMods, dstEnv, entry.type, srcMods, srcEnv));
          if (result.rank == ConversionRank::IDENTICAL) {
            break;
          }
        }
      }
      return result;
    }

    if (dst->kind == Type::Kind::CONTINGENT) {
      auto dstCont = static_cast<const ContingentType*>(dst);
      ConversionResult result(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
      for (auto& entry : dstCont->entries) {
        if (entry.when->isViable()) {
          result = result.better(isAssignable(entry.type, dstMods, dstEnv, src, srcMods, srcEnv));
          if (result.rank == ConversionRank::IDENTICAL) {
            break;
          }
        }
      }
      return result;
    }

//   if isinstance(dst, graph.ModifiedType):
//     with debug.indented():
//       debug.trace('Modified:')
//       if isConst(dst):
//         dstConst = True
//       return isAssignableEnv(dst.getBase(), dstConst, dstEnv, src, srcConst, srcEnv)

    if (dst->kind == Type::Kind::VOID) {
      if (src->kind == Type::Kind::VOID) {
        return ConversionResult(ConversionRank::IDENTICAL);
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::BOOLEAN) {
      if (src->kind == Type::Kind::BOOLEAN) {
        return ConversionResult(ConversionRank::IDENTICAL);
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::INTEGER) {
      auto dstInt = static_cast<const IntegerType*>(dst);
      if (src->kind == Type::Kind::INTEGER) {
        auto srcInt = static_cast<const IntegerType*>(src);
        if (srcInt->isImplicitlySized()) {
          if (dstInt->isUnsigned()) {
            // Requires one less bit for positive unsigned numbers.
            int32_t srcBits = srcInt->bits() - 1;
            if (srcInt->isNegative()) {
              return ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED);
            } else if (srcBits <= dstInt->bits()) {
              return ConversionResult(ConversionRank::EXACT);
            } else {
              return ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION);
            }
          } else if (srcInt->isUnsigned()) {
            // Can't assign unsigned to signed
            return ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED);
          } else if (dstInt->isImplicitlySized()) {
            // Implicitly sized can accept anything.
            return ConversionResult(ConversionRank::EXACT);
          } else if (srcInt->bits() <= dstInt->bits()) {
            return ConversionResult(ConversionRank::EXACT);
          } else {
            return ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION);
          }
        } else if (dstInt->isImplicitlySized()) {
          if (dstInt->isUnsigned() && !srcInt->isUnsigned()) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED);
          } else {
            // Number of bits in destination doesn't matter.
            return ConversionResult(ConversionRank::EXACT);
          }
        } else {
          if (dstInt->isUnsigned() != srcInt->isUnsigned()) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED);
          } else if (srcInt->bits() > dstInt->bits()) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION);
          } else if (srcInt->bits() < dstInt->bits()) {
            return ConversionResult(ConversionRank::EXACT);
          } else {
            return ConversionResult(ConversionRank::IDENTICAL);
          }
        }
      } else if (src->kind == Type::Kind::ENUM) {
        // Can assign enum to integer
        auto srcEnum = static_cast<const UserDefinedType*>(src);
        auto srcBase = cast<IntegerType>(cast<TypeDefn>(srcEnum->defn()->extends()[0])->type());
        auto result = isAssignable(dst, dstMods, dstEnv, srcBase, srcMods, srcEnv);
        if (result.rank == ConversionRank::IDENTICAL) {
          // It's not identical
          return ConversionResult(ConversionRank::EXACT);
        } else {
          return result;
        }
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::FLOAT) {
      if (src->kind == Type::Kind::FLOAT) {
        auto dstFloat = static_cast<const FloatType*>(dst);
        auto srcFloat = static_cast<const FloatType*>(src);
        if (srcFloat->bits() > dstFloat->bits()) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::PRECISION_LOSS);
        } else if (srcFloat->bits() < dstFloat->bits()) {
          return ConversionResult(ConversionRank::EXACT);
        } else {
          return ConversionResult(ConversionRank::IDENTICAL);
        }
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    // Identical User-defined types with identical type arguments.
    if (src == dst && srcEnv.args.size() == dstEnv.args.size()) {
      if (dst->kind == Type::Kind::CLASS ||
          dst->kind == Type::Kind::STRUCT ||
          dst->kind == Type::Kind::INTERFACE ||
          dst->kind == Type::Kind::TRAIT) {
        auto dstClass = static_cast<const UserDefinedType*>(dst);
        for (auto param : dstClass->defn()->allTypeParams()) {
          if (!isEqual(dstEnv.args[param->index()], srcEnv.args[param->index()])) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
          }
        }
        if (!isCompatibleMods(dstMods, srcMods)) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
        }
        return ConversionResult(ConversionRank::IDENTICAL);
      }
    }

    if (dst->kind == Type::Kind::CLASS) {
      if (src->kind == Type::Kind::CLASS) {
        // If src is a subtype of dst then it's OK.
        auto dstClass = static_cast<const UserDefinedType*>(dst);
        auto srcClass = static_cast<const UserDefinedType*>(src);
        if (isSubtype(srcClass->defn(), srcEnv, dstClass->defn(), dstEnv)) {
          if (!isCompatibleMods(dstMods, srcMods)) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
          }
          return ConversionResult(ConversionRank::EXACT);
        }
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::STRUCT) {
      // Structs only can be assigned to if they are identical; otherwise it would slice.
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::INTERFACE) {
      auto dstIface = static_cast<const UserDefinedType*>(dst);
      if (src->kind == Type::Kind::CLASS) {
        auto srcClass = static_cast<const UserDefinedType*>(src);
        // If src is a subtype of dst then it's OK.
        if (isSubtype(srcClass->defn(), srcEnv, dstIface->defn(), dstEnv)) {
          if (!isCompatibleMods(dstMods, srcMods)) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
          }
          return ConversionResult(ConversionRank::EXACT);
        } else if (implementsMembers(srcClass->defn(), srcEnv, dstIface->defn(), dstEnv)) {
          if (!isCompatibleMods(dstMods, srcMods)) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
          }
          return ConversionResult(ConversionRank::EXACT);
        }
        return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
      } else if (src->kind == Type::Kind::INTERFACE) {
        auto srcIface = static_cast<const UserDefinedType*>(src);
        // TODO: check extends list or structural typing
        // If src is a subtype of dst then it's OK.
        if (isSubtype(srcIface->defn(), srcEnv, dstIface->defn(), dstEnv)) {
          if (!isCompatibleMods(dstMods, srcMods)) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
          }
          return ConversionResult(ConversionRank::EXACT);
        }
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::TRAIT) {
      // Can't assign to a trait since you can't have a destination that is a trait.
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::ENUM) {
      // Should have already succeeded at this point.
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::TUPLE) {
      if (src->kind == Type::Kind::TUPLE) {
        auto dstTuple = static_cast<const TupleType*>(dst);
        auto srcTuple = static_cast<const TupleType*>(src);
        if (srcTuple->members.size() == dstTuple->members.size()) {
          ConversionResult result;
          for (size_t i = 0; i < srcTuple->members.size(); ++i) {
            result = result.worse(isAssignable(
                dstTuple->members[i], dstMods, dstEnv, srcTuple->members[i], srcMods, srcEnv));
            if (result.rank == ConversionRank::ERROR) {
              break;
            }
          }
          return result;
        }
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    if (dst->kind == Type::Kind::UNION) {
      // If the destination is a union, choose the BEST of all possible conversion results.
      auto dstUnion = static_cast<const UnionType*>(dst);
      ConversionResult result;
      for (auto memberType : dstUnion->members) {
        result = result.better(isAssignable(memberType, dstMods, dstEnv, src, srcMods, srcEnv));
        if (result.rank >= ConversionRank::EXACT) {
          break;
        }
      }
      // Can't get better than 'exact'.
      if (result.rank == ConversionRank::IDENTICAL) {
        return ConversionResult(ConversionRank::EXACT);
      }
      return result;
    }

    if (dst->kind == Type::Kind::FUNCTION) {
      if (src->kind == Type::Kind::FUNCTION) {
        auto dstFunc = static_cast<const FunctionType*>(dst);
        auto srcFunc = static_cast<const FunctionType*>(src);
        if (!dstFunc->isMutableSelf && srcFunc->isMutableSelf) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::QUALIFIER_LOSS);
        }
        if (dstFunc->isVariadic != srcFunc->isVariadic) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
        }
        assert(srcFunc->returnType);
        assert(dstFunc->returnType);
        if (!isEqual(dstFunc->returnType, 0, dstEnv, srcFunc->returnType, 0, srcEnv)) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
        }
        if (srcFunc->paramTypes.size() != dstFunc->paramTypes.size()) {
          return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
        }
        for (size_t i = 0; i < srcFunc->paramTypes.size(); i += 1) {
          if (!isEqual(dstFunc->paramTypes[i], 0, dstEnv, srcFunc->paramTypes[i], 0, srcEnv)) {
            return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
          }
        }
        return ConversionResult(ConversionRank::IDENTICAL);
      }
      return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
    }

    diag.fatal() << "isAssignable not handled: " << src->kind << " : " << dst->kind;
    assert(false && "isAssignable not handled");
    return ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE);
  }
}
