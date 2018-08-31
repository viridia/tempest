#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"

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

  void ConstraintSolver::addBindingConstraint(
      const source::Location& loc,
      const Type* dstType,
      const Type* srcType,
      BindingConstraint::Restriction restriction,
      Conditions when) {
  }

  void ConstraintSolver::addSite(OverloadSite* site) {
    site->ordinal = _sites.size();
    _sites.push_back(site);
  }

  void ConstraintSolver::run() {
//     if self.typeChoices:
// #       debug.write(list(self.typeChoices.values()))
//       self.sites.extend(self.typeChoices.values())
    findRankUpperBound();
    if (_failed) {
      return;
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

//     self.conversionConstraints = set([ct for ct in self.conversionConstraints if not ct.isIdentity()])
//     self.originalConversionConstraints = self.conversionConstraints
//     self.tvarConstraints.update(self.equivalenceConstraints)
//     for ct in self.tvarConstraints:
//       assert self.renamer.hasTypeVar(ct.typeVar)
//     self.unifyConstraints()
//     self.combineRejections()
//     self.chooseIntegerSizesForConstraints()
//     self.validateConstraints(self.tvarConstraints)

//     self.removeIdentityConstraints()

//     self.propagateConstraints()
//     self.combineConstraints()

//     self.removeAlwaysTrueConditions()
//     self.removeNonViableConstraints()
//     self.combineConstraints()

//     # TODO: This should probably be done in a loop, with convergence detection.
//     self.cullCandidatesByTypeConstraints()
//     self.cullCandidatesByRequirements()
//     if self.checkAmbiguousCallSites():
//       self.cullCandidatesByConversionRank()
//     if self.checkAmbiguousCallSites():
//       self.cullCandidatesBySpecificity()
//     self.cullCandidatesByRequirements()
//     self.removeAlwaysTrueConditions()
//     self.removeNonViableConstraints()
//     if self.tracing:
//       self.dump()
//       for site in self.sites:
//         self.reportCandidateStatus(site)

//     self.selectFinalCandidates()
//     self.removeAlwaysTrueConditions()
//     self.removeNonViableConstraints()
//     self.propagateConstraints()
//     self.combineConstraints()
//     self.removeNonViableConstraints()
//     self.computeUniqueValueForTypeVars()
//     callsite.Choice.tracing = False
  }

  void ConstraintSolver::updateConversionRanks() {
    // Reset all conversion results for candidates that have not been eliminated
  //   for (auto site : _sites) {
  //     for (auto oc : site->candidates) {
  //       if (oc->rejection.reason == Rejection::NONE) {
  //         // oc->
  // //         cc.detailedConversionResults = {}
  //       }
  //     }
  //   }

    // for (auto cct : _conversionConstraints) {
    // }

  //   for ac in self.conversionConstraints:
  //     if ac.isViable() and len(ac.when) > 0:
  //       self.updateConversionRankingsForConstraint(ac)

  //   # Collate results.
  //   for site in sites:
  //     for cc in site.candidates:
  //       cc.conversionResults.clear()
  //       for ac, result in cc.detailedConversionResults.items():
  //         # This algorithm is bogus but it works for the moment. Replace with something better.
  //         if len(ac.when) == 1:
  //           cc.conversionResults[result.rank] += 10
  //         else:
  //           # What we want to know is, is there any other assignmentConstraint that occupies
  //           # the same set of call sites as this one, but has a better conversion ranking?
  //           cc.conversionResults[result.rank] += 1
  }

  // def updateConversionRankingsForConstraint(self, ac):
  //   # Re-evaluate a constraint, and update the conversion rankings for all candidates that
  //   # support the constraint.

  //   # Disable any competitors to the candidates in this constraint
  //   for cc in ac.when:
  //     cc.site.pruneCandidates(True)
  //     cc.pruned = False

  //   # Compute the conversion result
  //   saveTracing = debug.tracing
  //   debug.tracing = False
  //   result = typerelation.isAssignable(ac.dstType, ac.srcType)
  //   debug.tracing = saveTracing
  //   if debug.tracing:
  //     debug.trace('Conversion rank for:', ac, 'is', result)
  //     for when in ac.when:
  //       debug.trace('  =>', when)

  //   # Assign blame to any candidates that this constraint depends on.
  //   for cc in ac.when:
  //     cc.detailedConversionResults[ac] = result

  //   # Re-enable candidates
  //   for cc in ac.when:
  //     cc.site.pruneCandidates(False)

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
      for (auto site : _sites) {
        if (site->kind == OverloadKind::CALL) {
          auto callSite = static_cast<CallSite*>(site);
          for (size_t i = 0; i < callSite->argTypes.size(); i += 1) {
            rejection = rejection || rejectErrorCandidates(callSite, i);
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

  bool ConstraintSolver::rejectErrorCandidates(CallSite* site, size_t argIndex) {
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

  void ConstraintSolver::reportSiteAmbiguities() {
    for (auto site : _sites) {
      if (site->allRejected()) {
        if (site->kind == OverloadKind::CALL) {
          auto callSite = static_cast<CallSite*>(site);
          diag.error(callSite->location) << "No suitable method for call:";
          diag.info() << "Possible methods are:";
          reportCandidateStatus(site);
        }
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
      diag.error(callSite->location) << "No method found for input arguments:";
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
            for (; lastRequiredParam < method->params().size(); ++lastRequiredParam) {
              auto p = method->params()[lastRequiredParam];
              if (p->init() != nullptr || p->isVariadic() || p->isKeywordOnly()) {
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
              if (p->isVariadic() || p->isKeywordOnly()) {
                break;
              }
            }

            diag.info(method->location()) << cc->method << ": expects no more than "
                << lastPositionalParam << " arguments, " << oc->rejection.argIndex
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

          // Type inference rejections
          // UNIFICATION_ERROR,
          // UNSATISFIED_REQIREMENT,
          // UNSATISFIED_TYPE_CONSTRAINT,
          // INCONSISTENT, // Contradictory constraints

          case Rejection::NOT_MORE_SPECIALIZED:
            diag.info(method->location()) << cc->method
                << ": matches less strictly than other candidates.";
            break;

          case Rejection::NOT_BEST:
            diag.info(method->location()) << cc->method
                << ": rejected because other choices produce fewer type conversionss.";
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
}
