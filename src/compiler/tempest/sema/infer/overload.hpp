#ifndef TEMPEST_SEMA_INFER_OVERLOAD_HPP
#define TEMPEST_SEMA_INFER_OVERLOAD_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_REJECTION_HPP
  #include "tempest/sema/infer/rejection.hpp"
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;

  class OverloadSite;

  /** Abstract base class for one of several overload candidates during constraint solving.
      Overloads can be functions calls or type specializations. The solver will attempt to
      choose the best candidate for each site. */
  class OverloadCandidate {
  public:
    /** True if this candidate has been eliminated from consideration. */
    bool pruned = false;

    /** True if this is the sole remaining candidate. */
    bool accepted = false;

    /** Why this candidate was rejected (if it was). */
    Rejection rejection;

    /** Site where this overload is invoked. */
    OverloadSite* site;

    // self.conversionResults = defaultdict(int)
    // self.detailedConversionResults = {}

  // def reject(self, reason):
  //   if self.accepted:
  //     debug.fail('Rejection of sole remaining candidate', self)
  //   self.rejection = reason
  //   if Choice.tracing:
  //     debug.write('reject:', self, reason=reason)

  // def isViable(self):
  //   return not self.rejection and not self.pruned

  // def hasBetterConversionRanks(self, other):
  //   return typerelation.isBetterConversionRanks(self.conversionResults, other.conversionResults)

  // @abstractmethod
  // def excludes(self, other):
  //   return False

  // def excludes(self, other):
  //   # Two candidates are exclusive if they are from the same call site.
  //   return self.site is other.site and self is not other

  };

  /** Overload candidate representing a function call. */
  class CallCandidate : public OverloadCandidate {
  public:
    /** Expression that evaluates to the callable. */
    Expr* callable;

    /** The definition being called. */
    Member* member;

    /** The type that is returned from the function. */
    Type* returnType;

    /** Indices that map argument lots to parameter slots. */
    llvm::SmallVector<size_t, 8> paramAssignments;
  };

  /** Overload candidate representing a generic type specialization. */
  class SpecializationCandidate : public OverloadCandidate {
  public:
    /** The generic type being specialized. */
    GenericDefn* member;
  };

  /** Abstract base class representing a set of candidates competing at a given site
      in the expression graph. */
  class OverloadSite {
  public:
  };

  class PredicateSet {

  };
}

#endif
