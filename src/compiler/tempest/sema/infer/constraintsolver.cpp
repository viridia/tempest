#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/unification.hpp"
#include "llvm/ADT/EquivalenceClasses.h"

namespace std {
  inline ::std::ostream& operator<<(::std::ostream& os, const std::vector<size_t>& perm) {
    os << "{";
    auto sep = "";
    for (auto i : perm) {
      os << sep << i;
      sep = ", ";
    }
    os << "}";
    return os;
  }
}

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::convert;
  using tempest::error::diag;

  void ConstraintSolver::addSite(OverloadSite* site) {
    site->ordinal = _sites.size();
    _sites.push_back(site);
  }

  void ConstraintSolver::run() {
    unifyConstraints();
    if (_failed) {
      return;
    }

    findRankUpperBound();
    if (_failed) {
      return;
    }

    if (!isSingularSolution()) {
      narrowPassRejection();
      if (_failed) {
        return;
      }
    }

    if (!isSingularSolution()) {
      findBestRankedOverloads();
      if (_failed) {
        return;
      }
    }

    if (!isSingularSolution()) {
      cullCandidatesBySpecificity();
    }

    if (!isSingularSolution()) {
      _failed = true;
      reportSiteAmbiguities();
    }

    if (!_failed) {
      checkNonCandidateConstraints();
      computeUniqueValueForTypeVars();
    }
  }

  void ConstraintSolver::unifyConstraints() {
    for (auto site : _sites) {
      if (auto callSite = dyn_cast<CallSite>(site)) {
        for (auto oc : site->candidates) {
          auto cc = static_cast<CallCandidate*>(oc);
          std::vector<UnificationResult> unificationResults;
          Conditions when;
          if (site->candidates.size() > 0) {
            when.add(callSite->ordinal, cc->ordinal);
          }
          for (size_t i = 0; i < callSite->argTypes.size() && !cc->isRejected(); i += 1) {
            auto paramIndex = cc->paramAssignments[i];
            if (!unify(
                unificationResults,
                cc->paramTypes[paramIndex],
                callSite->argTypes[i],
                when,
                TypeRelation::ASSIGNABLE_FROM,
                _alloc)) {
              cc->rejection.reason = Rejection::UNIFICATION_ERROR;
              cc->rejection.argIndex = i;
            }
          }

          for (auto& result : unificationResults) {
            result.param->constraints.push_back(
                InferredType::Constraint(result.value, result.predicate, result.conditions));
          }
        }
      }
    }

    for (auto& assign : _assignments) {
      Conditions when;
      std::vector<UnificationResult> unificationResults;
      if (!unify(
          unificationResults,
          assign.dstType,
          assign.srcType,
          when,
          TypeRelation::ASSIGNABLE_FROM,
          _alloc)) {
        diag.error(assign.location) << "Cannot convert type " << assign.srcType << " to "
            << assign.dstType << ".";
        _failed = true;
        break;
      }
      for (auto& result : unificationResults) {
        result.param->constraints.push_back(
            InferredType::Constraint(result.value, result.predicate, result.conditions));
      }
    }
  }

  bool ConstraintSolver::isViable(const Conditions& cond) {
    return std::all_of(
      cond.begin(), cond.end(),
      [cond, this](const Conditions::Conjunct& cj) {
        auto candidates = _sites[cj.site()]->candidates;
        for (auto choice : cj) {
          if (candidates[choice]->isViable()) {
            return true;
          }
        }
        return false;
      }
    );
  }

  bool ConstraintSolver::isSingularSolution() const {
    for (auto site : _sites) {
      if (!site->isSingular()) {
        return false;
      }
    }
    return true;
  }

  void ConstraintSolver::findRankUpperBound() {
    // Set up pruning flags
    for (auto site : _sites) {
      site->pruneAll(false);
    }

    // Attempt to remove all candidates from consideration that would result in a conversion
    // error.
    for (;;) {
      bool rejection = false;
      for (auto& constraint : _bindings) {
        if (constraint.candidate->isViable()) {
          if (constraint.predicate == TypeRelation::SUBTYPE) {
            if (!isEqualOrNarrower(constraint.dstType, constraint.srcType)) {
              constraint.candidate->rejection.reason = Rejection::UNSATISFIED_TYPE_CONSTRAINT;
              constraint.candidate->rejection.constraint = &constraint;
              rejection = true;
            }
          }
        }
      }

      for (auto site : _sites) {
        if (site->kind == OverloadKind::CALL) {
          auto callSite = static_cast<CallSite*>(site);
          for (size_t i = 0; i < callSite->argTypes.size(); i += 1) {
            rejection = rejection || rejectByParamAssignment(callSite, i);
          }
        } else {
          assert(false && "Implement overload kind");
        }

        if (site->allRejected()) {
          _failed = true;
          reportSiteRejections(site);
          return;
        }
      }

      if (!rejection) {
        break;
      }
    }

    // For each individual arg -> param assignment, find the best possible conversion rank.
    _bestRankings.clear();
    for (auto site : _sites) {
      if (site->kind == OverloadKind::CALL) {
        auto callSite = static_cast<CallSite*>(site);
        for (size_t i = 0; i < callSite->argTypes.size(); i += 1) {
          ConversionResult resultForArg = paramConversion(callSite, i);
          _bestRankings.count[int(resultForArg.rank)] += 1;
        }
      } else {
        assert(false && "Implement overload kind");
      }
    }
  }

  bool ConstraintSolver::rejectByParamAssignment(CallSite* site, size_t argIndex) {
    bool rejection = false;
    for (auto oc : site->candidates) {
      if (oc->isViable()) {
        auto cc = static_cast<CallCandidate*>(oc);
        auto paramIndex = cc->paramAssignments[argIndex];
        auto result = isAssignable(cc->paramTypes[paramIndex], site->argTypes[argIndex]);
        if (result.rank == ConversionRank::ERROR) {
          rejection = true;
          if (result.error == ConversionError::QUALIFIER_LOSS) {
            cc->rejection.reason = Rejection::Reason::QUALIFIER_LOSS;
          } else {
            cc->rejection.reason = Rejection::Reason::CONVERSION_FAILURE;
          }
          cc->rejection.argIndex = argIndex;
        }
      }
    }
    return rejection;
  }

  void ConstraintSolver::narrowPassRejection() {
    // Set up pruning flags
    for (auto site : _sites) {
      site->pruneAll(false);
    }

    for (auto site : _sites) {
      if (site->candidates.size() > 1) {
        // All viable candidates are unpruned, except for this one site.
        site->pruneAll(true);
        for (auto oc : site->candidates) {
          if (oc->rejection.reason != Rejection::NONE) {
            continue;
          }

          oc->pruned = false;

          // So the conversion rankings represent the best possible case if this candidate is
          // chosen.

          for (auto typeArg : oc->typeArgs) {
            if (oc->rejection.reason != Rejection::NONE) {
              break;
            }
            if (auto inferred = dyn_cast<InferredType>(typeArg)) {
              if (oc->rejection.reason != Rejection::NONE) {
                break;
              }

              // See if explicit condition conflicts with assignment
              for (auto& excon : _bindings) {
                if (excon.candidate == oc && excon.dstType == inferred) {
                  for (auto& constraint : inferred->constraints) {
                    if (oc->rejection.reason != Rejection::NONE) {
                      break;
                    }
                    if (!isViable(constraint.when)) {
                      continue;
                    }

                    if (excon.predicate == TypeRelation::SUBTYPE) {
                      if (constraint.predicate == TypeRelation::ASSIGNABLE_FROM) {
                        if (isAssignable(excon.srcType, constraint.value).rank
                                == ConversionRank::ERROR) {
                          oc->rejection.reason = Rejection::CONFLICT;
                          oc->rejection.constraint = &excon;
                          oc->rejection.itype = inferred;
                          oc->rejection.iconst0 = &constraint;
                          break;
                        }
                      }
                    }
                  }
                }
              }

              // See if we can find any contradictory type constraints
              for (auto& constraint : inferred->constraints) {
                if (oc->rejection.reason != Rejection::NONE) {
                  break;
                }
                if (!isViable(constraint.when)) {
                  continue;
                }

                for (auto& constraint2 : inferred->constraints) {
                  if (&constraint == &constraint2) {
                    continue;
                  }
                  if (!isViable(constraint.when)) {
                    continue;
                  }

                  if (constraint.when == constraint2.when) {
                    if (constraint.predicate == TypeRelation::ASSIGNABLE_FROM) {
                      if (constraint2.predicate == TypeRelation::ASSIGNABLE_FROM) {
                        if (isAssignable(constraint.value, constraint2.value).rank
                                == ConversionRank::ERROR &&
                            isAssignable(constraint2.value, constraint.value).rank
                                == ConversionRank::ERROR) {
                          oc->rejection.reason = Rejection::INCONSISTENT;
                          oc->rejection.itype = inferred;
                          oc->rejection.iconst0 = &constraint;
                          oc->rejection.iconst1 = &constraint2;
                          break;
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          oc->pruned = true;
        }
        site->pruneAll(false);
        if (site->allRejected()) {
          reportSiteRejections(site);
          _failed = true;
          return;
        }
      }
    }
  }

  void ConstraintSolver::findBestRankedOverloads() {
    // Set up pruning flags
    _currentPermutation.resize(_sites.size());
    _bestPermutationCount = 0;
    _bestPermutation.resize(_sites.size());
    _bestRankings.setToWorst();
    for (auto site : _sites) {
      site->pruneAll(true);
    }
    findBestRankedOverloads(_sites, 0);
    for (auto site : _sites) {
      site->pruneAll(false);
    }

    // diag.debug() << "Best permutation count: " << _bestPermutationCount;
    if (_bestRankings.isError()) {
      _failed = true;
      diag.error(location) << "Unable to find a type solution for expression.";
      // diag.info() << "Best overload candidates are:";
      // TODO: Need error test
      // Set up pruning flags and also list best overload
      // for (auto site : _sites) {
      //   Member* member = site->candidates[_bestPermutation[site->ordinal]]->getMember();
      //   diag.info(unwrapSpecialization(member)->getLocation()) << member;
      //   site->pruneAllExcept(_bestPermutation[site->ordinal]);
      // }
    } else {
      // Go through and reject any candidate that wasn't part of a 'best' solution.
      for (auto site : _sites) {
        for (auto oc : site->candidates) {
          if (!_bestPermutationSet.count((site->ordinal << 16) | oc->ordinal)) {
            oc->rejection.reason = Rejection::NOT_BEST;
          }
        }
      }
    }
  }

  void ConstraintSolver::findBestRankedOverloads(
      const llvm::ArrayRef<OverloadSite*>& sites, size_t index) {
    if (index < sites.size()) {
      auto site = sites[index];
      for (size_t i = 0; i < site->candidates.size(); i += 1) {
        auto oc = site->candidates[i];
        if (oc->rejection.reason != Rejection::NONE) {
          continue;
        }
        oc->pruned = false;
        _currentPermutation[index] = i;
        findBestRankedOverloads(sites, index + 1);
        oc->pruned = true;
      }
    } else {
      ConversionRankTotals rankings = computeRankingsForConfiguration();
      if (rankings.isBetterThan(_bestRankings)) {
        _bestPermutationSet.clear();
        _bestRankings = rankings;
        _bestPermutationCount = 1;
        for (size_t i = 0; i < _currentPermutation.size(); i += 1) {
          _bestPermutationSet.insert((i << 16) | _currentPermutation[i]);
        }
      } else if (rankings.equals(_bestRankings)) {
        _bestPermutationCount += 1;
        for (size_t i = 0; i < _currentPermutation.size(); i += 1) {
          _bestPermutationSet.insert((i << 16) | _currentPermutation[i]);
        }
      }
    }
  }

  ConversionRankTotals ConstraintSolver::computeRankingsForConfiguration() {
    ConversionRankTotals rankings;
    for (auto site : _sites) {
      assert(site->numViable() == 1);
      if (site->kind == OverloadKind::CALL) {
        auto callSite = static_cast<CallSite*>(site);
        auto cc = static_cast<CallCandidate*>(
            site->candidates[_currentPermutation[site->ordinal]]);
        assert(cc->isViable());
        for (size_t i = 0; i < callSite->argTypes.size(); i += 1) {
          auto paramIndex = cc->paramAssignments[i];
          auto result = isAssignable(cc->paramTypes[paramIndex], callSite->argTypes[i]);
          rankings.count[int(result.rank)] += 1;
        }
      } else {
        assert(false && "Implement overload kind");
      }
    }
    for (auto& assign : _assignments) {
      auto result = isAssignable(assign.dstType, assign.srcType);
      rankings.count[int(result.rank)] += 1;
    }
    for (auto& constraint : _bindings) {
      if (constraint.candidate->isViable()) {
        if (constraint.predicate == TypeRelation::SUBTYPE) {
          if (!isEqualOrNarrower(constraint.dstType, constraint.srcType)) {
            rankings.count[int(ConversionRank::ERROR)] += 1;
          }
        }
      }
    }
    return rankings;
  }

  ConversionResult ConstraintSolver::paramConversion(CallSite* site, size_t argIndex) {
    ConversionResult paramResult;
    for (auto oc : site->candidates) {
      if (oc->isViable()) {
        auto cc = static_cast<CallCandidate*>(oc);
        auto paramIndex = cc->paramAssignments[argIndex];
        auto result = isAssignable(cc->paramTypes[paramIndex], site->argTypes[argIndex]);
        paramResult = paramResult.better(result);
      }
    }
    return paramResult;
  }

  void ConstraintSolver::cullCandidatesBySpecificity() {
    for (auto site : _sites) {
      if (site->kind == OverloadKind::CALL) {
        SmallVector<CallCandidate*, 8> preserved;
        SmallVector<CallCandidate*, 8> rejected;
        SmallVector<CallCandidate*, 8> mostSpecific;
        for (auto oc : site->candidates) {
          if (oc->isRejected()) {
            continue;
          }
          CallCandidate* cc = static_cast<CallCandidate*>(oc);
          bool addNew = true;
          // Try each candidate against each of the others. Reject any candidate that is
          // more general than another candidate.
          for (auto msCandidate : mostSpecific) {
            // TODO: If the return type is not constrained, then we don't want to consider the
            // return type when doing the comparison.
            if (cc->isEqualOrNarrower(msCandidate)) {
              if (msCandidate->isEqualOrNarrower(cc)) {
                // They are the same specificity, keep both.
                preserved.push_back(msCandidate);
              } else {
                // cc is better
                rejected.push_back(msCandidate);
              }
            } else {
              preserved.push_back(msCandidate);
              if (msCandidate->isEqualOrNarrower(cc)) {
                // ms is better
                addNew = false;
              }
              // else neither is better, so keep both.
            }
          }

          mostSpecific.swap(preserved);
          preserved.clear();
          if (addNew) {
            mostSpecific.push_back(cc);
          }
        }

        if (mostSpecific.size() == 1) {
          for (auto rej : rejected) {
            rej->rejection.reason = Rejection::NOT_MORE_SPECIALIZED;
          }
        }
      } else {
        assert(false && "Implement OC Spec");
      }
    }
  }

  void ConstraintSolver::reportSiteAmbiguities() {
    for (auto site : _sites) {
      if (site->allRejected()) {
        reportSiteRejections(site);
      } else if (!site->isSingular()) {
        if (site->kind == OverloadKind::CALL) {
          auto callSite = static_cast<CallSite*>(site);
          diag.error(callSite->location) << "Ambiguous overloaded method:";
          diag.info() << "Possible methods are:";
          reportCandidateStatus(site);
        }
      }
    }
  }

  void ConstraintSolver::reportSiteRejections(OverloadSite* site) {
    if (site->kind == OverloadKind::CALL) {
      auto callSite = static_cast<CallSite*>(site);
      diag.error(callSite->location) << "No suitable method found for call:";
      diag.info() << "Possible methods are:";
      reportCandidateStatus(site);
    } else {
      assert(false && "Support other site types");
    }
  }

  void ConstraintSolver::reportCandidateStatus(OverloadSite* site) {
    if (site->kind == OverloadKind::CALL) {
      auto callSite = static_cast<CallSite*>(site);
      for (auto oc : callSite->candidates) {
        auto cc = static_cast<CallCandidate*>(oc);
        auto method = cast<FunctionDefn>(unwrapSpecialization(cc->method));
        switch (oc->rejection.reason) {
          case Rejection::NONE:
            diag.info(method->location()) << cc->method;
            break;

          case Rejection::NOT_ENOUGH_ARGS: {
            size_t lastRequiredParam = 0;
            size_t numParams =
                cc->isVariadic ? method->params().size() - 1 : method->params().size();
            for (; lastRequiredParam < numParams; ++lastRequiredParam) {
              auto p = method->params()[lastRequiredParam];
              if (p->init() != nullptr || p->isKeywordOnly()) {
                break;
              }
            }

            if (oc->rejection.argIndex == 0) {
              diag.info(method->location()) << cc->method << ": requires at least "
                  << lastRequiredParam << " arguments, none were supplied.";

            } else if (oc->rejection.argIndex == 1) {
              diag.info(method->location()) << cc->method << ": requires at least "
                  << lastRequiredParam << " arguments, only 1 was supplied.";
            } else {
              diag.info(method->location()) << cc->method << ": requires at least "
                  << lastRequiredParam << " arguments, only " << oc->rejection.argIndex
                  << " were supplied.";
            }
            break;
          }

          case Rejection::TOO_MANY_ARGS: {
            size_t lastPositionalParam = 0;
            for (; lastPositionalParam < method->params().size(); ++lastPositionalParam) {
              auto p = method->params()[lastPositionalParam];
              if (p->isKeywordOnly()) {
                break;
              }
            }

            diag.info(method->location()) << cc->method << ": expects no more than "
                << lastPositionalParam << " arguments, " << callSite->argList.size()
                << " were supplied.";
            break;
          }

          case Rejection::KEYWORD_NOT_FOUND:
            diag.info(method->location()) << cc->method << ": keyword not found.";
            break;

          case Rejection::KEYWORD_IN_USE:
            diag.info(method->location()) << cc->method << ": keyword already used.";
            break;

          case Rejection::KEYWORD_ONLY_ARG:
            diag.info(method->location()) << cc->method << ": keyword only arg.";
            break;

          case Rejection::UNIFICATION_ERROR:
          case Rejection::CONVERSION_FAILURE: {
            auto paramIndex = cc->paramAssignments[oc->rejection.argIndex];
            auto srcType = callSite->argTypes[oc->rejection.argIndex];
            auto dstType = cc->paramTypes[paramIndex];
            diag.info(method->location()) << cc->method << ": cannot convert argument "
                << (oc->rejection.argIndex + 1) << " from " << srcType
                << " to " << dstType << ".";
            break;
          }

          case Rejection::QUALIFIER_LOSS: {
            auto paramIndex = cc->paramAssignments[oc->rejection.argIndex];
            auto srcType = callSite->argTypes[oc->rejection.argIndex];
            auto dstType = cc->paramTypes[paramIndex];
            diag.info(method->location()) << cc->method << ": conversion of argument "
                << (oc->rejection.argIndex + 1) << " from " << srcType
                << " to " << dstType << " would lose qualifiers.";
            break;
          }

          case Rejection::UNSATISFIED_TYPE_CONSTRAINT: {
            auto constraint = oc->rejection.constraint;
            auto srcType = constraint->srcType;
            if (auto srcInferred = dyn_cast<InferredType>(srcType)) {
              srcType = srcInferred->typeParam->typeVar();
            }
            auto dstType = oc->rejection.constraint->dstType;
            diag.info(method->location()) << cc->method << ": template parameter "
                << constraint->param->name() << " cannot be bound to type "
                << ShowConstraints(dstType) << ", it must be a subtype of " << srcType << ".";
            break;
          }

          // Type inference rejections
          // UNSATISFIED_REQIREMENT,

          case Rejection::NOT_MORE_SPECIALIZED:
            diag.info(method->location()) << cc->method
                << ": matches less strictly than other candidates.";
            break;

          case Rejection::INCONSISTENT:
            diag.info(method->location()) << cc->method
                << ": no type found for " << oc->rejection.itype->typeParam
                << " that satisfies all the following constraints:";
            if (oc->rejection.iconst0->predicate == TypeRelation::ASSIGNABLE_FROM) {
              diag.info() << "  assignable from " << oc->rejection.iconst0->value;
            } else {
              assert(false && "Implement");
            }

            if (oc->rejection.iconst1->predicate == TypeRelation::ASSIGNABLE_FROM) {
              diag.info() << "  assignable from " << oc->rejection.iconst1->value;
            } else {
              assert(false && "Implement");
            }
            break;

          case Rejection::CONFLICT:
            diag.info(method->location()) << cc->method
                << ": no type found for " << oc->rejection.itype->typeParam
                << " that satisfies all the following constraints:";
            if (oc->rejection.constraint->predicate == TypeRelation::SUBTYPE) {
              diag.info() << "  subtype of " << oc->rejection.constraint->srcType;
            } else {
              assert(false && "Implement");
            }
            if (oc->rejection.iconst0->predicate == TypeRelation::ASSIGNABLE_FROM) {
              diag.info() << "  assignable from " << oc->rejection.iconst0->value;
            } else {
              assert(false && "Implement");
            }
            break;

          case Rejection::NOT_BEST:
            diag.info(method->location()) << cc->method
                << ": rejected because other choices produce fewer type conversions.";
            break;

          default:
            assert(false && "Format reason");
        }
      }
    }
  }

  void ConstraintSolver::reportConversionError(
      const Type* dst,
      const Type* src,
      ConversionResult result) {
    if (result.rank == ConversionRank::ERROR) {
      switch (result.error) {
        case ConversionError::INCOMPATIBLE:
          diag.error() << "Can't convert from " << src << " to " << dst << ".";
          break;

        case ConversionError::QUALIFIER_LOSS:
          diag.error() << "Loss of qualifiers converting from " << src << " to " << dst << ".";
          break;

        default:
          assert(false && "Bad conversion error rank");
      }
    } else if (result.rank == ConversionRank::WARNING) {
      switch (result.error) {
        case ConversionError::TRUNCATION:
          diag.error() << "Truncation of value converting from " << src << " to " << dst << ".";
          break;

        case ConversionError::SIGNED_UNSIGNED:
          diag.error() << "Signed / unsigned mismatch converting from " << src
              << " to " << dst << ".";
          break;

        case ConversionError::PRECISION_LOSS:
          diag.error() << "Loss of precision converting from " << src << " to " << dst << ".";
          break;

        default:
          assert(false && "Bad conversion error rank");
      }
    }
  }

  void ConstraintSolver::checkNonCandidateConstraints() {
    for (auto assign : _assignments) {
      auto result = isAssignable(assign.dstType, assign.srcType);
      if (result.rank <= ConversionRank::WARNING) {
        switch (result.error) {
          case ConversionError::INCOMPATIBLE:
            diag.error(assign.location)
                << "Can't convert from " << assign.srcType << " to " << assign.dstType << ".";
            break;
          case ConversionError::QUALIFIER_LOSS:
            diag.error(assign.location)
                << "Conversion from " << assign.srcType << " to " << assign.dstType
                << " loses qualifiers.";
            break;
          case ConversionError::SIGNED_UNSIGNED:
            diag.error(assign.location)
                << "Signed / unsigned mismatch converting from " << assign.srcType
                << " to " << assign.dstType << ".";
            break;
          case ConversionError::TRUNCATION:
            diag.warn(assign.location)
                << "Truncation of value converting from " << assign.srcType
                << " to " << assign.dstType << ".";
            break;
          case ConversionError::PRECISION_LOSS:
            diag.warn(assign.location)
                << "Loss of precision converting from " << assign.srcType
                << " to " << assign.dstType << ".";
            break;
          case ConversionError::NONE:
            break;
        }
      }
    }
  }

  void ConstraintSolver::computeUniqueValueForTypeVars() {
    llvm::EquivalenceClasses<const InferredType*> ec;

    // Find all type variables that are equivalent.
    for (auto site : _sites) {
      auto oc = site->singularCandidate();
      assert(oc != nullptr);
      for (auto typeArg : oc->typeArgs) {
        if (auto inferred = dyn_cast_or_null<InferredType>(typeArg)) {
          inferred->value = nullptr;
          ec.insert(inferred);
          for (auto& constraint : inferred->constraints) {
            if (inferred->isViable(constraint)) {
              if (auto value = dyn_cast<InferredType>(constraint.value)) {
                ec.unionSets(inferred, value);
              }
            }
          }
        }
      }
    }

    // For each equivalent set, find all of the constraints
    for (auto it = ec.begin(); it != ec.end(); ++it) {
      const Type* equivalent = nullptr;
      const Type* assignableFrom = nullptr;
      const Type* assignableTo = nullptr;
      for (auto mit = ec.member_begin(it); mit != ec.member_end(); ++mit) {
        auto inferred = *mit;
        for (auto& constraint : inferred->constraints) {
          if (inferred->isViable(constraint)) {
            if (constraint.value->kind != Type::Kind::INFERRED) {
              if (constraint.predicate == TypeRelation::EQUAL) {
                // If there are multiple equal constraints, they must be equal to each other.
                if (!equivalent) {
                  equivalent = constraint.value;
                } else if (!isEqual(equivalent, constraint.value)) {
                  assert(false && "Inconsistent");
                }
              } else if (constraint.predicate == TypeRelation::ASSIGNABLE_FROM) {
                // If there are multiple assignableFroms, then pick the more general one,
                // that is, the one that can be assigned from all the others. If they are
                // disjoint, that's an error.
                if (!assignableFrom) {
                  assignableFrom = constraint.value;
                } else if (isAssignable(assignableFrom, constraint.value).rank ==
                    ConversionRank::ERROR) {
                  if (isAssignable(constraint.value, assignableFrom) ==
                      ConversionRank::ERROR) {
                    assert(false && "Inconsistent");
                  } else {
                    assignableFrom = constraint.value;
                  }
                }
              } else if (constraint.predicate == TypeRelation::ASSIGNABLE_TO) {
                // If there are multiple assignableTos, then pick the more specific one,
                // that is, the one that can be assigned to all the others. If they are
                // disjoint, that's an error.
                if (!assignableTo) {
                  assignableTo = constraint.value;
                } else if (isAssignable(constraint.value, assignableTo).rank ==
                    ConversionRank::ERROR) {
                  if (isAssignable(assignableTo, constraint.value) ==
                      ConversionRank::ERROR) {
                    assert(false && "Inconsistent");
                  } else {
                    assignableTo = constraint.value;
                  }
                }
              } else {
                // TODO: other constraint types.
                assert(false && "Invalid predicate");
              }
            }
          }
        }
      }

      if (equivalent) {
        if (assignableFrom) {
          if (isAssignable(equivalent, assignableFrom).rank == ConversionRank::ERROR) {
            assert(false && "Inconsistent");
          }
        }
        if (assignableTo) {
          if (isAssignable(equivalent, assignableFrom).rank == ConversionRank::ERROR) {
            assert(false && "Inconsistent");
          }
        }
      } else if (assignableFrom) {
        equivalent = assignableFrom;
        if (assignableTo) {
          assert(false && "Implement");
        }
      } else if (assignableTo) {
        equivalent = assignableTo;
      } else {
        assert(false && "Implement");
      }

      if (auto intType = dyn_cast<IntegerType>(equivalent)) {
        if (intType->isImplicitlySized()) {
          if (intType->isUnsigned()) {
            if (intType->bits() - 1 <= 32) {
              equivalent = &IntegerType::U32;
            } else if (intType->bits() - 1 <= 64) {
              equivalent = &IntegerType::U64;
            } // Else leave it, handle truncation error later
          } else {
            if (intType->bits() <= 32) {
              equivalent = &IntegerType::I32;
            } else if (intType->bits() <= 64) {
              equivalent = &IntegerType::I64;
            } // Else leave it, handle truncation error later
          }
        }
      }

      for (auto mit = ec.member_begin(it); mit != ec.member_end(); ++mit) {
        auto inferred = *mit;
        inferred->value = equivalent;
      }
    }
  }
}
