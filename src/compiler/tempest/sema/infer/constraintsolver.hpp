#ifndef TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP
#define TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP 1

#ifndef TEMPEST_SEMA_INFER_CONSTRAINT_HPP
  #include "tempest/sema/infer/constraint.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_OVERLOAD_HPP
  #include "tempest/sema/infer/overload.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_RENAMER_HPP
  #include "tempest/sema/infer/renamer.hpp"
#endif

#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
  #include "tempest/sema/convert/result.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  class GenericDefn;
}

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using tempest::sema::convert::ConversionResult;

  /** Base class for constraints to be solved. */
  class ConstraintSolver {
  public:
    source::Location location;

    ConstraintSolver(
        const source::Location& location
        // GenericDefn* enclosingGeneric
        )
      : location(location)
      , _renamer(*this)
      // , _enclosingGeneric(enclosingGeneric)
    {}

    ~ConstraintSolver() {
      for (auto site : _sites) {
        delete site;
      }
      for (auto bc : _bindingConstraints) {
        delete bc;
      }
    }

    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    bool empty() const {
      return _bindingConstraints.empty()
          && _sites.empty();
    //   return and len(self.anonFns) == 0
    }

    TypeVarRenamer& renamer() { return _renamer; }

    void addBindingConstraint(
        const source::Location& loc,
        const Type* dstType,
        const Type* srcType,
        BindingConstraint::Restriction restriction,
        Conditions when);

    void addSite(OverloadSite* site);

    void run();
    bool failed() const { return _failed; }
    void updateConversionRanks();

    /** Find the best possible conversion rankings. */
    void findRankUpperBound();

    /** Brute-force search of all overload permutations to find the best ranking. */
    void findBestRankedOverloads();
    void findBestRankedOverloads(const llvm::ArrayRef<OverloadSite*>& sites, size_t index);

    void cullCandidatesBySpecificity();

    /** True if a set of conditions has not yet been precluded by overload pruning. */
    bool isViable(const Conditions& cond);
    bool isSingularSolution() const;

  private:
    tempest::support::BumpPtrAllocator _alloc;
    TypeVarRenamer _renamer;
    // GenericDefn* _enclosingGeneric;
    std::vector<BindingConstraint*> _bindingConstraints;
    std::vector<OverloadSite*> _sites;
    std::vector<size_t> _currentPermutation;
    std::vector<size_t> _bestPermutation;
    std::unordered_set<uint32_t> _bestPermutationSet;
    size_t _bestPermutationCount;
    ConversionRankTotals _bestRankings;
    bool _failed = false;

    /** Find the best possible conversion for the nth parameter at a call site, checking
        all remaining viable overloads. */
    ConversionResult paramConversion(CallSite* site, size_t argIndex);
    bool rejectErrorCandidates(CallSite* site, size_t argIndex);

    void reportSiteAmbiguities();
    void reportSiteRejections(OverloadSite* errorSite);
    void reportCandidateStatus(OverloadSite* site);
    void reportConversionError(const Type* dst, const Type* src, ConversionResult result);
  };
}

#endif
