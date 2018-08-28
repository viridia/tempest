#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using namespace tempest::sema::convert;
  using tempest::error::diag;

  void ConstraintSolver::addAssignment(
      const source::Location& loc,
      const Type* dstType,
      const Type* srcType,
      Conditions when) {
    (void)_alloc;
    diag.debug() << "Assignment: " << srcType << " -> " << dstType;

    if (Type::isError(srcType) || Type::isError(dstType)) {
      return;
    }

    assert (srcType->kind != Type::Kind::NOT_EXPR);
    assert (dstType->kind != Type::Kind::NOT_EXPR);
    assert (srcType->kind != Type::Kind::NEVER);
    assert (dstType->kind != Type::Kind::NEVER);

    if (dstType != srcType) {
      _conversionConstraints.push_back(new ConversionConstraint(when, dstType, srcType));
    }
//     if (srcType->kind == Type::Kind::CONTINGENT) {
//       // This expands contingent types into separate constraints with their own when-lists.
//       // However, it doesn't help the case where the types contain type variables which
//       // might also be ambiguous.
//       auto contingent = static_cast<const ContingentType*>(srcType);
//       for (auto entry : contingent->entries) {
//         Conditions entryConditions(entry.when->site->ordinal, entry.when->ordinal);
//         addAssignment(loc, dstType, entry.type, entryConditions);
//       }
// //     elif isinstance(srcType, graph.PhiType):
// //       for ty in srcType.getTypes():
// //         self.addAssignment(location, dstType, ty, *when)
//     } else {
//       if (dstType != srcType) {
//         _conversionConstraints.push_back(new ConversionConstraint(when, dstType, srcType));
//       }
//     }
  }

  void ConstraintSolver::addBindingConstraint(
      const source::Location& loc,
      const Type* dstType,
      const Type* srcType,
      BindingConstraint::Restriction restriction,
      Conditions when) {
  }

  void ConstraintSolver::run() {
//     for site in self.sites:
//       for candidate in site.candidates:
//         if isinstance(candidate, callsite.CallCandidate) and candidate.member:
//           for traceSubject in self.TO_TRACE:
//             if traceSubject.match(candidate.member, self.location):
//               self.tracing = True
//               callsite.Choice.tracing = True
//               break

//     if self.typeChoices:
// #       debug.write(list(self.typeChoices.values()))
//       self.sites.extend(self.typeChoices.values())
    findRankUpperBound();
    if (_failed) {
      return;
    }
    findBestRankedOverloads();
    if (_failed) {
      return;
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

  void ConstraintSolver::findRankUpperBound() {
    // Set up pruning flags
    for (auto site : _sites) {
      site->pruneAll(false);
    }
    _bestRankings.clear();
    for (auto cct : _conversionConstraints) {
      if (!isViable(cct->when)) {
        continue;
      }
      auto result = isAssignable(cct->dstType, cct->srcType);
      diag.debug() << "isAssignable: " << cct->srcType << " -> " << cct->dstType
          << ": " << result;
      _bestRankings.count[int(result.rank)] += 1;
    }
    if (_bestRankings.isError()) {
      diag.error(location) << "Type conversion error:";
      _failed = true;
    }
  }

  void ConstraintSolver::findBestRankedOverloads() {
    // Set up pruning flags
    for (auto site : _sites) {
      site->pruneAll(true);
    }
    _currentPermutation.resize(_sites.size());
    _bestPermutation.resize(_sites.size());
    _bestRankings.setToWorst();
    findBestRankedOverloads(_sites, 0);
    if (_bestRankings.isError()) {
      _failed = true;
      diag.error(location) << "Unable to find a type solution for expression.";
      diag.info() << "Best overload candidates are:";
      // Set up pruning flags and also list best overload
      for (auto site : _sites) {
        Member* member = site->candidates[_bestPermutation[site->ordinal]]->getMember();
        diag.info(unwrapSpecialization(member)->getLocation()) << member;
        site->pruneAllExcept(_bestPermutation[site->ordinal]);
      }

      for (auto cct : _conversionConstraints) {
        if (!isViable(cct->when)) {
          continue;
        }
        auto result = isAssignable(cct->dstType, cct->srcType);
        if (result.rank == ConversionRank::ERROR) {
          switch (result.error) {
            case ConversionError::INCOMPATIBLE:
              diag.error() << "Can't convert from " << cct->srcType << " to " << cct->dstType;
              break;

            case ConversionError::QUALIFIER_LOSS:
              diag.error() << "Loss of qualifiers converting from " << cct->srcType << " to "
                  << cct->dstType;
              break;

            default:
              assert(false && "Bad conversion error rank");
          }
        } else if (result.rank == ConversionRank::WARNING) {
          switch (result.error) {
            case ConversionError::TRUNCATION:
              diag.error() << "Truncation of value converting from " << cct->srcType << " to "
                  << cct->dstType;
              break;

            case ConversionError::SIGNED_UNSIGNED:
              diag.error() << "Signed / unsigned mismatch converting from " << cct->srcType
                  << " to " << cct->dstType;
              break;

            case ConversionError::PRECISION_LOSS:
              diag.error() << "Loss of precision converting from " << cct->srcType
                  << " to " << cct->dstType;
              break;

            default:
              assert(false && "Bad conversion error rank");
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
        oc->pruned = false;
        _currentPermutation[index] = i;
        findBestRankedOverloads(sites, index + 1);
        oc->pruned = true;
      }
    } else {
      ConversionRankTotals rankings;
      for (auto cct : _conversionConstraints) {
        auto result = isAssignable(cct->dstType, cct->srcType);
        rankings.count[int(result.rank)] += 1;
      }

      if (rankings.isBetterThan(_bestRankings)) {
        _bestRankings = rankings;
      }
    }
  }
}
