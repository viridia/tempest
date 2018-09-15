#ifndef TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP
#define TEMPEST_SEMA_INFER_CONSTRAINTSOLVER_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_OVERLOAD_HPP
  #include "tempest/sema/infer/overload.hpp"
#endif

#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
  #include "tempest/sema/convert/result.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::graph {
  class GenericDefn;
  class BinaryOp;
}

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using tempest::sema::convert::ConversionResult;

  /** Overloads can only be chosen which don't violate a constraint. */
  struct AssignmentConstraint {
    source::Location location;
    const Type* dstType;
    const Type* srcType;
  };

  /** Explicit constraints on a type parameter. These must always hold regardless of
      which overloads are chosen.
   */
  struct ExplicitConstraint {
    TypeParameter* param;
    const Type* dstType;
    const Type* srcType;
    TypeRelation predicate;
    OverloadCandidate* candidate;
  };

  /** Base class for constraints to be solved. */
  class ConstraintSolver {
  public:
    source::Location location;

    ConstraintSolver(
        const source::Location& location
        // GenericDefn* enclosingGeneric
        )
      : location(location)
      // , _enclosingGeneric(enclosingGeneric)
    {}

    ~ConstraintSolver() {
      for (auto site : _sites) {
        delete site;
      }
    }

    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    bool empty() const {
      return _bindings.empty() && _sites.empty();
    //   return and len(self.anonFns) == 0
    }

    const std::vector<OverloadSite*>& sites() const { return _sites; }

    void addAssignment(
        const source::Location& loc,
        const Type* dstType,
        const Type* srcType) {
      _assignments.push_back({ loc, dstType, srcType });
    }

    void addExplicitConstraint(
        TypeParameter* param,
        const Type* dstType,
        const Type* srcType,
        TypeRelation predicate,
        OverloadCandidate* candidate) {
      _bindings.push_back({ param, dstType, srcType, predicate, candidate });
    }

    void addSite(OverloadSite* site);

    void run();
    bool failed() const { return _failed; }
    void unifyConstraints();
    void computeUniqueValueForTypeVars();

    /** Find the best possible conversion rankings. */
    void findRankUpperBound();

    /** Reject candidates by restricting a single site to each candidate in turn. */
    void narrowPassRejection();

    /** Brute-force search of all overload permutations to find the best ranking. */
    void findBestRankedOverloads();
    void findBestRankedOverloads(const llvm::ArrayRef<OverloadSite*>& sites, size_t index);

    void cullCandidatesBySpecificity();

    /** True if a set of conditions has not yet been precluded by overload pruning. */
    bool isViable(const Conditions& cond);
    bool isSingularSolution() const;

  private:
    tempest::support::BumpPtrAllocator _alloc;
    std::vector<AssignmentConstraint> _assignments;
    std::vector<ExplicitConstraint> _bindings;
    std::vector<OverloadSite*> _sites;
    std::vector<size_t> _currentPermutation;
    std::vector<size_t> _bestPermutation;
    std::unordered_set<uint32_t> _bestPermutationSet;
    size_t _bestPermutationCount;
    ConversionRankTotals _bestRankings;
    bool _failed = false;

    ConversionRankTotals computeRankingsForConfiguration();

    /** Find the best possible conversion for the nth parameter at a call site, checking
        all remaining viable overloads. */
    ConversionResult paramConversion(CallSite* site, size_t argIndex);
    bool rejectByParamAssignment(CallSite* site, size_t argIndex);

    void reportSiteAmbiguities();
    void reportSiteRejections(OverloadSite* errorSite);
    void reportCandidateStatus(OverloadSite* site);
    void reportConversionError(const Type* dst, const Type* src, ConversionResult result);
  };
}

#endif
