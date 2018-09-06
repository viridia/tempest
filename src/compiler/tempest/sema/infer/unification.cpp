#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/env.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/infer/unification.hpp"
#include "tempest/sema/transform/applyspec.hpp"

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::convert;
  using tempest::error::diag;
  using tempest::sema::transform::ApplySpecialization;

  bool isNonGeneric(const Type* t) {
    return (t->kind >= Type::Kind::VOID && t->kind <= Type::Kind::FLOAT) ||
        t->kind == Type::Kind::ENUM;
  }

  bool separateInferredMembers(
      llvm::SmallVectorImpl<const Type*>& members,
      Env& env,
      const InferredType*& inferred) {
    for (auto it = members.begin(); it != members.end(); ) {
      auto m = *it;
      if (m->kind == Type::Kind::TYPE_VAR) {
        auto typeVar = static_cast<const TypeVar*>(m);
        if (size_t(typeVar->index()) < env.args.size()) {
          m = env.args[typeVar->index()];
        }
      }
      if (m->kind == Type::Kind::INFERRED) {
        if (inferred) {
          return false;
        }
        inferred = static_cast<const InferredType*>(m);
        members.erase(it);
      } else {
        it++;
      }
    }
    return true;
  }

  // Unify a union type with a non-union type
  bool unifyUnion(
    std::vector<UnificationResult>& result,
    const UnionType* lt, Env& ltEnv,
    const Type* rt, Env& rtEnv,
    Conditions& when,
    BindingPredicate predicate,
    tempest::support::BumpPtrAllocator& alloc) {

    llvm::SmallVector<const Type*, 8> members(lt->members.begin(), lt->members.end());
    const InferredType* inferred = nullptr;

    // Remove inferred variables from union
    auto part = std::partition(
        members.begin(), members.end(),
        [](auto t) { return t->kind != Type::Kind::INFERRED; });
    if (auto count = std::distance(part, members.end())) {
      if (count > 1) {
        // More than two inferred variables, won't work
        return false;
      }
      inferred = static_cast<const InferredType*>(*part);
      members.erase(part);
    }

    if (predicate == BindingPredicate::ASSIGNABLE_FROM ||
        predicate == BindingPredicate::SUPERTYPE) {
      // In order for a union to be assignable from X, at least one member of the union
      // must be able to unify with X.
      for (auto member : members) {
        if (unify(result, member, ltEnv, rt, rtEnv, when, predicate, alloc)) {
          return true;
        }
      }
      return false;
    } else if (predicate == BindingPredicate::ASSIGNABLE_TO ||
        predicate == BindingPredicate::SUBTYPE) {
      // In order for a union to be assignable to X, all members of the union
      // must be able to unify with X.
      std::vector<UnificationResult> memberResult;
      for (auto member : members) {
        if (!unify(memberResult, member, ltEnv, rt, rtEnv, when, predicate, alloc)) {
          // Don't keep member result if failed
          return false;
        }
      }

      // Do keep member result if succeeded.
      result.insert(result.end(), memberResult.begin(), memberResult.end());
      return true;
    } else {
      assert(false && "Bad binding predicate");
    }
  }

  bool unify(
    std::vector<UnificationResult>& result, const Type* lt, const Type* rt, Conditions& when,
    BindingPredicate predicate, tempest::support::BumpPtrAllocator& alloc)
  {
    Env ltEnv;
    Env rtEnv;
    return unify(result, lt, ltEnv, rt, rtEnv, when, predicate, alloc);
  }

  bool unify(
    std::vector<UnificationResult>& result,
    const Type* lt, Env& ltEnv,
    const Type* rt, Env& rtEnv,
    Conditions& when,
    BindingPredicate predicate,
    tempest::support::BumpPtrAllocator& alloc)
  {
    // Modified types - merely strip the modifier. Const and immutable don't prevent unification,
    // but they can create a conversion error later in the process.
    if (lt->kind == Type::Kind::MODIFIED) {
      lt = static_cast<const ModifiedType*>(lt)->base;
    }

    if (rt->kind == Type::Kind::MODIFIED) {
      rt = static_cast<const ModifiedType*>(rt)->base;
    }

    // A type variable, if it shows up here, is a non-inferred type variable, which means that
    // it's type should be known, no need to unify.
    if (lt->kind == Type::Kind::TYPE_VAR) {
      if (lt == rt && ltEnv.args.empty() && rtEnv.args.empty()) {
        return true;
      }
      auto ltTypeVar = static_cast<const TypeVar*>(lt);
      if (size_t(ltTypeVar->index()) < ltEnv.args.size()) {
        Env newEnv;
        assert(ltEnv.has(ltTypeVar));
        return unify(
            result, ltEnv.args[ltTypeVar->index()], newEnv, rt, rtEnv, when, predicate, alloc);
      }
      auto ltParam = ltTypeVar->param;
      if (!ltParam->subtypeConstraints().empty()) {
        assert(false && "Implement subtype constraints");
      }
      return false;
    }

    if (rt->kind == Type::Kind::TYPE_VAR) {
      auto rtTypeVar = static_cast<const TypeVar*>(rt);
      if (size_t(rtTypeVar->index()) < rtEnv.args.size()) {
        Env newEnv;
        assert(rtEnv.has(rtTypeVar));
        return unify(
            result, lt, ltEnv, rtEnv.args[rtTypeVar->index()], newEnv, when, predicate, alloc);
      }
      auto rtParam = rtTypeVar->param;
      if (!rtParam->subtypeConstraints().empty()) {
        assert(false && "Implement subtype constraints");
      }
      return false;
    }

    // If we see a specialized, expand it's type args and recurse with the new env.
    if (lt->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(lt);
      ApplySpecialization apply(ltEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return unify(result, genType, newEnv, rt, rtEnv, when, predicate, alloc);
    }

    if (rt->kind == Type::Kind::SPECIALIZED) {
      auto sp = static_cast<const SpecializedType*>(rt);
      ApplySpecialization apply(rtEnv.args);
      Env newEnv;
      newEnv.params = sp->spec->typeParams();
      for (auto param : newEnv.params) {
        auto typeArg = sp->spec->typeArgs()[param->index()];
        newEnv.args.push_back(apply.transform(typeArg));
      }
      auto genType = cast<TypeDefn>(sp->spec->generic())->type();
      return unify(result, lt, ltEnv, genType, newEnv, when, predicate, alloc);
    }

    // Inferred types - add to unification results.
    if (lt->kind == Type::Kind::INFERRED) {
      auto infType = static_cast<const InferredType*>(lt);
      result.push_back({ const_cast<InferredType*>(infType), rt, predicate, when });
      return true;
    }

    if (rt->kind == Type::Kind::INFERRED) {
      auto infType = static_cast<const InferredType*>(rt);
      result.push_back({
          const_cast<InferredType*>(infType), lt, inversePredicate(predicate), when });
      return true;
    }

    // Primitive types handled just as regular type predicates. Note that we don't care
    // at this point what the 'other side' of the relation is. We know that no variables will
    // be added to the unification result because these types aren't generic.
    if ((isNonGeneric(rt) && lt->kind != Type::Kind::UNION) ||
        (isNonGeneric(lt) && rt->kind != Type::Kind::UNION)) {
      if (lt == rt) {
        return true;
      }

      switch (predicate) {
        case BindingPredicate::EQUAL:
          return isEqual(lt, 0, ltEnv, rt, 0, rtEnv);
        case BindingPredicate::ASSIGNABLE_FROM:
          return isAssignable(lt, 0, ltEnv, rt, 0, rtEnv).rank > ConversionRank::ERROR;
        case BindingPredicate::ASSIGNABLE_TO:
          return isAssignable(rt, 0, rtEnv, lt, 0, ltEnv).rank > ConversionRank::ERROR;
        case BindingPredicate::SUBTYPE:
          return isEqualOrNarrower(lt, ltEnv, rt, rtEnv);
        case BindingPredicate::SUPERTYPE:
          return isEqualOrNarrower(rt, rtEnv, lt, ltEnv);
      }
    }

//     if isinstance(left, graph.TypeVar):
//       assert left.hasIndex(), debug.format(left, right)
// #       assert left not in leftEnv
//       if leftEnvs:
//         assert False
//       leftParam = left.getParam()
//       if isinstance(right, graph.TypeVar):
//         # TODO: Unify against parameter constraints
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(left, self.applyEnv(right, rightEnvs), when)]
//         return [AssignableFromConstraint(left, self.applyEnv(right, rightEnvs), when)]

//       if self.occurs(right, leftParam):
//         return []
//       if isinstance(right, (graph.PrimitiveType, graph.Enum)):
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(left, right, when)]
//         return [AssignableFromConstraint(left, right, when)]
//       elif isinstance(right, (graph.Composite, graph.TupleType, graph.FunctionType)):
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(left, self.applyEnv(right, rightEnvs), when)]
//         return [AssignableFromConstraint(left, self.applyEnv(right, rightEnvs), when)]
//       elif isinstance(right, graph.UnionType):
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(left, self.applyEnv(right, rightEnvs), when)]
//         return [SubtypeConstraint(left, self.applyEnv(right, rightEnvs), when)]

//     if isinstance(right, graph.TypeVar):
//       if not right.hasIndex():
//         assert rightEnvs
//         env, *envList = rightEnvs
//         assert right in env
//         return self.unifyAssignment(left, leftEnvs, env[right], envList, when)
//       elif right.context:
//         when = when | {right.context}
//       if rightEnvs:
//         assert right not in rightEnvs[0], debug.format(left, leftEnvs, right, rightEnvs)
//         return []
//       rightParam = right.getParam()
//       if self.occurs(left, rightParam):
//         return []
//       if isinstance(left, (graph.PrimitiveType, graph.Enum)):
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(right, left, when)]
//         return [AssignableToConstraint(right, left, when)]
//       elif isinstance(left, (graph.Composite, graph.TupleType, graph.UnionType, graph.FunctionType)):
//         if op == UnifyOp.EQUIVALENT:
//           return [EquivalenceConstraint(right, self.applyEnv(left, leftEnvs), when)]
//         return [AssignableToConstraint(right, self.applyEnv(left, leftEnvs), when)]

//       assert False, type(left)

    // So:
    //  * (T | void) <-> (S | void)
    //  * (T | void) <-> (S | int)
    //  * (T | void) <-> (S | int | void)
    if (lt->kind == Type::Kind::UNION) {
      auto ltUnion = static_cast<const UnionType*>(lt);
      if (rt->kind == Type::Kind::UNION) {
        auto rtUnion = static_cast<const UnionType*>(rt);
        llvm::SmallVector<const Type*, 8> ltMembers(
            ltUnion->members.begin(), ltUnion->members.end());
        llvm::SmallVector<const Type*, 8> rtMembers(
            rtUnion->members.begin(), rtUnion->members.end());
        const InferredType* ltInferred = nullptr;
        const InferredType* rtInferred = nullptr;

        // Remove inferred variables from each union
        if (!separateInferredMembers(ltMembers, ltEnv, ltInferred)) {
          return false;
        }
        if (!separateInferredMembers(rtMembers, rtEnv, rtInferred)) {
          return false;
        }

        // Now find all the types which are common to both, unification-wise.
        // This algorithm assumes that each side has at most one matching entry on the
        // other side. We really ought to discard all matching entries.
        std::vector<UnificationResult> memberResult;
        for (auto lit = ltMembers.begin(); lit != ltMembers.end(); ) {
          auto ltMember = *lit;
          auto rit = std::find_if(rtMembers.begin(), rtMembers.end(),
              [&memberResult, ltMember, &ltEnv, &rtEnv, &when, predicate, &alloc]
              (const Type* rtMember) {
                return unify(memberResult, ltMember, ltEnv,
                    rtMember, rtEnv, when, predicate, alloc);
              });

          // If there's a match, remove from both sides
          if (rit != rtMembers.end()) {
            rtMembers.erase(rit);
            ltMembers.erase(lit);
          } else {
            lit++;
          }
        }

        if (!ltMembers.empty()) {
          // Leftover entries, but no variable to bind them to
          if (rtInferred) {
            if (ltMembers.size() == 1) {
              memberResult.push_back({
                  const_cast<InferredType*>(rtInferred), ltMembers[0],
                  inversePredicate(predicate), when });
            } else {
              auto tempUnion = new (alloc) UnionType(alloc.copyOf(ltMembers));
              memberResult.push_back({
                  const_cast<InferredType*>(rtInferred), tempUnion,
                  inversePredicate(predicate), when });
            }
            ltMembers.clear();
          }
        } else if (rtInferred && (
            predicate == BindingPredicate::EQUAL ||
            predicate == BindingPredicate::ASSIGNABLE_FROM ||
            predicate == BindingPredicate::SUPERTYPE)) {
          // Right hand inferred type member is unknown
          return false;
        }

        if (!rtMembers.empty()) {
          if (ltInferred) {
            if (rtMembers.size() == 1) {
              memberResult.push_back({
                  const_cast<InferredType*>(ltInferred), rtMembers[0], predicate, when });
            } else {
              auto tempUnion = new (alloc) UnionType(alloc.copyOf(rtMembers));
              memberResult.push_back({
                  const_cast<InferredType*>(ltInferred), tempUnion, predicate, when });
            }
            rtMembers.clear();
          }
        } else if (ltInferred && (
            predicate == BindingPredicate::EQUAL ||
            predicate == BindingPredicate::ASSIGNABLE_TO ||
            predicate == BindingPredicate::SUBTYPE)) {
          // Left hand inferred type member is unknown
          return false;
        }

        // If there's anything remaining on the left side:
        if (!ltMembers.empty() && (
            predicate == BindingPredicate::ASSIGNABLE_TO ||
            predicate == BindingPredicate::SUBTYPE ||
            predicate == BindingPredicate::EQUAL)) {
          return false;
        }

        // If there's anything remaining on the right side:
        if (!rtMembers.empty() && (
            predicate == BindingPredicate::ASSIGNABLE_FROM ||
            predicate == BindingPredicate::SUPERTYPE ||
            predicate == BindingPredicate::EQUAL)) {
          return false;
        }

        result.insert(result.end(), memberResult.begin(), memberResult.end());
        return true;
      }

      if (predicate == BindingPredicate::EQUAL) {
        return false;
      } else {
        return unifyUnion(result, ltUnion, ltEnv, rt, rtEnv, when, predicate, alloc);
      }
    }

    if (rt->kind == Type::Kind::UNION) {
      if (predicate == BindingPredicate::EQUAL) {
        return false;
      } else {
        auto rtUnion = static_cast<const UnionType*>(rt);
        return unifyUnion(
            result, rtUnion, rtEnv, lt, ltEnv, when, inversePredicate(predicate), alloc);
      }
    }

    // (T, bool) <=> (S, bool)
    if (lt->kind == Type::Kind::TUPLE) {
      if (rt->kind == Type::Kind::TUPLE) {
        auto ltTuple = static_cast<const TupleType*>(lt);
        auto rtTuple = static_cast<const TupleType*>(rt);
        if (ltTuple->members.size() == rtTuple->members.size()) {
          std::vector<UnificationResult> memberResult;
          auto memberPredicate = predicate;
          if (memberPredicate == BindingPredicate::SUBTYPE ||
              memberPredicate == BindingPredicate::SUPERTYPE) {
            memberPredicate = BindingPredicate::EQUAL;
          }
          for (size_t i = 0; i < ltTuple->members.size(); i += 1) {
            if (!unify(memberResult,
                ltTuple->members[i], ltEnv,
                rtTuple->members[i], rtEnv, when, memberPredicate, alloc)) {
              return false;
            }
          }

          // Add member results to overall results and succeed
          result.insert(result.end(), memberResult.begin(), memberResult.end());
          return true;
        }
      }
      return false;
    }

    if (rt->kind == Type::Kind::TUPLE) {
      return false;
    }

    if (lt->kind == Type::Kind::CLASS ||
        lt->kind == Type::Kind::STRUCT ||
        lt->kind == Type::Kind::INTERFACE ||
        lt->kind == Type::Kind::TRAIT)
    {
      if (lt == rt) {
        auto udt = static_cast<const UserDefinedType*>(lt);
        auto td = udt->defn();
        std::vector<UnificationResult> paramResult;
        for (auto param : td->allTypeParams()) {
          if (!unify(paramResult,
              param->typeVar(), ltEnv,
              param->typeVar(), rtEnv, when, BindingPredicate::EQUAL, alloc)) {
            return false;
          }
        }

        // Add param results to overall results and succeed
        result.insert(result.end(), paramResult.begin(), paramResult.end());
        return true;
      }

      // Check base classes.
      if (rt->kind == Type::Kind::CLASS ||
          rt->kind == Type::Kind::STRUCT ||
          rt->kind == Type::Kind::INTERFACE ||
          rt->kind == Type::Kind::TRAIT) {
        if (predicate == BindingPredicate::SUBTYPE ||
            predicate == BindingPredicate::ASSIGNABLE_TO) {
          auto udt = static_cast<const UserDefinedType*>(lt);
          auto td = udt->defn();
          // Keep results from first match
          for (auto base : td->extends()) {
            auto baseType = base->kind == Member::Kind::SPECIALIZED ?
                static_cast<const SpecializedDefn*>(base)->type() : cast<TypeDefn>(base)->type();
            if (unify(result, baseType, ltEnv, rt, rtEnv, when, predicate, alloc)) {
              return true;
            }
          }
          for (auto base : td->implements()) {
            auto baseType = base->kind == Member::Kind::SPECIALIZED ?
                static_cast<const SpecializedDefn*>(base)->type() : cast<TypeDefn>(base)->type();
            if (unify(result, baseType, ltEnv, rt, rtEnv, when, predicate, alloc)) {
              return true;
            }
          }
        }
        if (predicate == BindingPredicate::SUPERTYPE ||
            predicate == BindingPredicate::ASSIGNABLE_FROM) {
          auto udt = static_cast<const UserDefinedType*>(rt);
          auto td = udt->defn();
          // Keep results from first match
          for (auto base : td->extends()) {
            auto baseType = base->kind == Member::Kind::SPECIALIZED ?
                static_cast<const SpecializedDefn*>(base)->type() : cast<TypeDefn>(base)->type();
            if (unify(result, lt, ltEnv, baseType, rtEnv, when, predicate, alloc)) {
              return true;
            }
          }
          for (auto base : td->implements()) {
            auto baseType = base->kind == Member::Kind::SPECIALIZED ?
                static_cast<const SpecializedDefn*>(base)->type() : cast<TypeDefn>(base)->type();
            if (unify(result, lt, ltEnv, baseType, rtEnv, when, predicate, alloc)) {
              return true;
            }
          }
        }
      }
      return false;
    }

//     if isinstance(right, graph.Composite):
//       return []
//       # Cases we have to deal with: coercion
//       # For that we probably want to pre-compute all possible conversions
//       assert False, debug.format(type(left), left, right)
//       pass

//     if isinstance(left, graph.FunctionType):
//       if isinstance(right, graph.FunctionType):
//         if len(left.getParamTypes()) != len(right.getParamTypes()):
//           return UnificationError(left, right)
//         result = []
//         for lParam, rParam in zip(left.getParamTypes(), right.getParamTypes()):
//           # TODO: This should be unifyEquivalent
//           unifyResult = self.unifyAssignment(
//               lParam, leftEnvs, rParam, rightEnvs, when, UnifyOp.EQUIVALENT)
//           if isinstance(unifyResult, UnificationError):
//             return unifyResult
//           result.extend(unifyResult)
//         unifyResult = self.unifyAssignment(
//             left.getReturnType(), leftEnvs,
//             right.getReturnType(), rightEnvs, when, UnifyOp.EQUIVALENT)
//         if isinstance(unifyResult, UnificationError):
//           return unifyResult
//         result.extend(unifyResult)
//         return result

// #     return UnificationError(left, right)
//     assert False, debug.format(left, '<==>', right, type(left), type(right))

    diag.fatal() << "Unify not handled: " << lt->kind << " : " << rt->kind;
    assert(false && "Unify not handled");
    return false;
  }
}
