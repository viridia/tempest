#ifndef TEMPEST_SEMA_INFER_OVERLOAD_HPP
#define TEMPEST_SEMA_INFER_OVERLOAD_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
  #include "tempest/sema/convert/result.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_REJECTION_HPP
  #include "tempest/sema/infer/rejection.hpp"
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

#ifndef LLVM_SUPPORT_CASTING_H
  #include <llvm/Support/Casting.h>
#endif

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;
  using tempest::sema::convert::ConversionRankTotals;

  class OverloadSite;

  enum class OverloadKind {
    CALL,
    SPECIALIZE,
  };

  /** Abstract base class for one of several overload candidates during constraint solving.
      Overloads can be functions calls or type specializations. The solver will attempt to
      choose the best candidate for each site. */
  class OverloadCandidate {
  public:

    /** What kind of overload this is. */
    const OverloadKind kind;

    /** True if this candidate has been eliminated from consideration. */
    bool pruned = false;

    /** True if this is the sole remaining candidate. */
    bool accepted = false;

    /** Why this candidate was rejected (if it was). */
    Rejection rejection;

    /** Index of this choice within the set of choices for the site. */
    size_t ordinal;

    /** Site where this overload is invoked. */
    OverloadSite* site;

    /** Summary of conversion results for this candidate. */
    ConversionRankTotals conversionResults;

    OverloadCandidate(OverloadKind kind, size_t ordinal, OverloadSite* site)
      : kind(kind)
      , ordinal(ordinal)
      , site(site) {}
    virtual ~OverloadCandidate() {}

    virtual Member* getMember() const = 0;

    // self.conversionResults = defaultdict(int)
    // self.detailedConversionResults = {}

  // def reject(self, reason):
  //   if self.accepted:
  //     debug.fail('Rejection of sole remaining candidate', self)
  //   self.rejection = reason
  //   if Choice.tracing:
  //     debug.write('reject:', self, reason=reason)

    /** True if this candidate is still being considered. */
    bool isViable() const { return rejection.reason == Rejection::NONE && !pruned; }

    /** True if this candidate has been rejected. */
    bool isRejected() const { return rejection.reason != Rejection::NONE; }

  // def hasBetterConversionRanks(self, other):
  //   return typerelation.isBetterConversionRanks(self.conversionResults, other.conversionResults)

  // @abstractmethod
  // def excludes(self, other):
  //   return False

  // def excludes(self, other):
  //   # Two candidates are exclusive if they are from the same call site.
  //   return self.site is other.site and self is not other

    /** Dynamic casting support. */
    static bool classof(const OverloadCandidate* t) { return true; }
  };

  /** Overload candidate representing a function call. */
  class CallCandidate : public OverloadCandidate {
  public:
    /** Expression that evaluates to the callable. */
    Expr* callable = nullptr;

    /** The definition being called. */
    Member* method;

    /** List of function parameters. */
    llvm::ArrayRef<ParameterDefn*> params;

    /** List of function parameter types. */
    llvm::ArrayRef<const Type*> paramTypes;

    /** The type that is returned from the function. */
    const Type* returnType = nullptr;

    /** Indices that map argument lots to parameter slots. */
    llvm::SmallVector<size_t, 8> paramAssignments;

    CallCandidate(OverloadSite* site, size_t ordinal, Member* member)
      : OverloadCandidate(OverloadKind::CALL, ordinal, site)
      , method(member)
    {}

    virtual Member* getMember() const { return method; }

    /** Return true if this candidate is either more specific or has equal specificity. */
    bool isEqualOrNarrower(CallCandidate* cc);

    /** Dynamic casting support. */
    static bool classof(const CallCandidate* t) { return true; }
    static bool classof(const OverloadCandidate* oc) { return oc->kind == OverloadKind::CALL; }
  };

  /** Overload candidate representing a generic type specialization. */
  class SpecializationCandidate : public OverloadCandidate {
  public:
    /** The generic being specialized. */
    GenericDefn* generic;

    virtual Member* getMember() const { return generic; }
  };

  /** Abstract base class representing a set of candidates competing at a given site
      in the expression graph. */
  class OverloadSite {
  public:
    /** What kind of overload this is. */
    const OverloadKind kind;
    source::Location location;
    std::vector<OverloadCandidate*> candidates;
    size_t ordinal;

    OverloadSite(OverloadKind kind, const source::Location& location)
      : kind(kind)
      , location(location)
    {}

    ~OverloadSite() {
      for (auto oc : candidates) {
        delete oc;
      }
    }

    void pruneAll(bool pruned) {
      for (auto oc : candidates) {
        oc->pruned = pruned;
      }
    }

    void pruneAllExcept(size_t index) {
      for (auto oc : candidates) {
        oc->pruned = true;
      }
      candidates[index]->pruned = false;
    }

    bool allRejected() const {
      return std::find_if(
          candidates.begin(),
          candidates.end(),
          [](auto oc) { return oc->rejection.reason == Rejection::NONE; }) == candidates.end();
    }

    size_t numViable() const {
      size_t result = 0;
      for (auto oc : candidates) {
        if (oc->isViable()) {
          result += 1;
        }
      }
      return result;
    }

    /** True if there is only a single remaining unrejected candidate. */
    bool isSingular() const {
      return singularCandidate() != nullptr;
    }

    /** Return the single remaining unrejected candidate, or null. */
    OverloadCandidate* singularCandidate() const {
      OverloadCandidate* result = nullptr;
      for (auto oc : candidates) {
        if (oc->rejection.reason == Rejection::NONE) {
          if (result) {
            return nullptr;
          } else {
            result = oc;
          }
        }
      }
      return result;
    }
  };

  /** An overload site for a method call. */
  class CallSite : public OverloadSite {
  public:
    Expr* callExpr;
    llvm::SmallVector<Expr*, 8> argList;
    const llvm::SmallVector<Type*, 8> argTypes;

    CallSite(
        const source::Location& location,
        Expr* callExpr,
        const llvm::ArrayRef<Expr*> &argList,
        llvm::ArrayRef<Type*> argTypes)
      : OverloadSite(OverloadKind::CALL, location)
      , callExpr(callExpr)
      , argList(argList.begin(), argList.end())
      , argTypes(argTypes.begin(), argTypes.end())
    {}

    CallCandidate* addCandidate(Member* method) {
      auto cc = new CallCandidate(this, candidates.size(), method);
      candidates.push_back(cc);
      return cc;
    }

    // def addExprCandidate(self, callableExpr):
    //   cc = CallCandidate(self, None, callableExpr)
    //   self.candidates.append(cc)
    //   return cc

    /** Dynamic casting support. */
    static bool classof(const CallSite* cs) { return true; }
    static bool classof(const OverloadSite* cs) { return cs->kind == OverloadKind::CALL; }
  };
}

#endif
