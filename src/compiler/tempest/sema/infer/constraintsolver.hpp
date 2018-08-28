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

namespace tempest::sema::graph {
  class GenericDefn;
}

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;

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
      for (auto cc : _conversionConstraints) {
        delete cc;
      }
      for (auto bc : _bindingConstraints) {
        delete bc;
      }
    }

    llvm::BumpPtrAllocator& alloc() { return _alloc; }

    bool empty() const {
      return _conversionConstraints.empty()
          && _bindingConstraints.empty()
          && _sites.empty();
    //   return and len(self.anonFns) == 0
    }

    TypeVarRenamer& renamer() { return _renamer; }

    void addAssignment(
        const source::Location& loc,
        const Type* dstType,
        const Type* srcType,
        Conditions when);

    void addBindingConstraint(
        const source::Location& loc,
        const Type* dstType,
        const Type* srcType,
        BindingConstraint::Restriction restriction,
        Conditions when);

    void addSite(OverloadSite* site) {
      site->ordinal = _sites.size();
      _sites.push_back(site);
    }

    void run();
    bool failed() const { return _failed; }
    void updateConversionRanks();

    /** Find the best possible conversion rankings. */
    void findRankUpperBound();

    /** Brute-force search of all overload permutations to find the best ranking. */
    void findBestRankedOverloads();
    void findBestRankedOverloads(const llvm::ArrayRef<OverloadSite*>& sites, size_t index);

    /** True if a set of conditions has not yet been precluded by overload pruning. */
    bool isViable(const Conditions& cond);

  private:
    llvm::BumpPtrAllocator _alloc;
    TypeVarRenamer _renamer;
    // GenericDefn* _enclosingGeneric;
    std::vector<ConversionConstraint*> _conversionConstraints;
    std::vector<BindingConstraint*> _bindingConstraints;
    std::vector<OverloadSite*> _sites;
    std::vector<size_t> _currentPermutation;
    std::vector<size_t> _bestPermutation;
    ConversionRankTotals _bestRankings;
    bool _failed = false;
  };
}

#endif
