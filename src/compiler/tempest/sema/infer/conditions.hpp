#ifndef TEMPEST_SEMA_INFER_CONDITIONS_HPP
#define TEMPEST_SEMA_INFER_CONDITIONS_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#include <initializer_list>

namespace tempest::sema::infer {
  class OverloadCandidate;
  class ConstraintSolver;

  /** Class that represents the set of conditions that must be satisfied in order for
      a particular type constraint to be applicable. These are expressed in the form of
      a boolean expression represented as ANDs (conjuncts) and ORs (disjuncts), in the form:

        (T1 OR T2 OR T3) AND (T4 OR T5) ...etc...

        (T1 OR T2 OR T3) AND (T4 OR T5) ...etc...

      The individual terms of the condition (Tn) represent the choice of which overload
      candidate is chosen for a particular call site. To make it easier to test this class,
      terms are stored as integer indices: (site index, candidate index). The determination
      as to whether a given term is true or false will depend on context and is the resposibility
      of the caller.

      During type inference, the set of overloads may initially be ambiguous, which means that
      multiple overloads may be chosen for a single site, but eventually those choices must be
      narrowed to a single overload per site.

      Not all overload sites will be included in a given condition set, since not every
      overload site affects whether a given type constraint is applicable.

      This class supports logical operations on condition sets. Adding a conjunct to a set
      makes it more restrictive, while adding choices within a conjunct makes it less
      restrictive.

      Because sets are typically very small (1 or 2 elements), we use a sorted SmallVector to store
      the entries and do linear searches for membership tests rather than a fancy unordered
      set.
  */
  class Conditions {
  public:
    /** The set of choices for a given overload site. */
    class Conjunct {
    public:
      typedef SmallVector<size_t, 2> Choices;
      typedef Choices::const_iterator const_iterator;

      Conjunct(size_t site) : _site(site) {}
      Conjunct(size_t site, size_t choice) : _site(site) { add(choice); }
      Conjunct(const Conjunct& src) : _site(src._site), _choices(src._choices) {}
      Conjunct(Conjunct&& src) : _site(src._site), _choices(std::move(src._choices)) {}
      Conjunct(std::initializer_list<size_t> src) {
        addAll(llvm::ArrayRef<size_t>(src));
      }

      size_t site() const { return _site; }

      bool empty() const {
        return _choices.empty();
      }

      size_t size() const {
        return _choices.size();
      }

      const_iterator begin() const { return _choices.begin(); }
      const_iterator end() const { return _choices.end(); }

      Conjunct& operator=(const Conjunct& src) {
        _site = src._site;
        _choices.assign(src._choices.begin(), src._choices.end());
        return *this;
      }

      Conjunct& operator=(Conjunct&& src) {
        _site = src._site;
        _choices = std::move(src._choices);
        return *this;
      }

      bool contains(size_t candidate) const {
        return std::find(_choices.begin(), _choices.end(), candidate) != _choices.end();
      }

      /** True if this set equals another conjunct. */
      bool equal(const Conjunct& other) const {
        return other._site == _site &&
            std::equal(
                _choices.begin(), _choices.end(),
                other._choices.begin(), other._choices.end());
      }

      bool operator==(const Conjunct& other) const {
        return equal(other);
      }

      /** True if this set includes all the conditions of `subset`. */
      bool isSuperset(const Conjunct& subset) const {
        return _site == subset._site &&
            std::includes(
                _choices.begin(), _choices.end(),
                subset._choices.begin(), subset._choices.end());
      }

      /** True if this set is a subset of `superset`. */
      bool isSubset(const Conjunct& superset) const {
        return _site == superset._site &&
            std::includes(
                superset._choices.begin(), superset._choices.end(),
                _choices.begin(), _choices.end());
      }

      /** Add elements to the set. Maintains sorted order. */
      bool add(size_t choice) {
        if (!contains(choice)) {
          _choices.insert(std::lower_bound(_choices.begin(), _choices.end(), choice), choice);
          return true;
        }
        return false;
      }

      /** Add a collection of elements to the set. */
      void addAll(const Conjunct& src) {
        for (auto choice : src._choices) {
          add(choice);
        }
      }

      void addAll(llvm::ArrayRef<size_t> src) {
        for (auto choice : src) {
          add(choice);
        }
      }

      /** Retains only the elements that are in both sets. */
      void intersectWith(const Conjunct& src) {
        _choices.erase(std::remove_if(
            _choices.begin(),
            _choices.end(),
            [src](size_t choice) { return !src.contains(choice); }));
      }

      /** True if there is at least one viable choice in this conjunct. */
      bool isViable(const ConstraintSolver& cs);

    private:
      /** Which site the choices apply to. */
      size_t _site;

      /** The set of valid choices for those sites. */
      Choices _choices;
    };

    typedef SmallVector<Conjunct, 3> Conjuncts;
    typedef Conjuncts::iterator iterator;
    typedef Conjuncts::const_iterator const_iterator;

    Conditions() {}
    Conditions(const Conditions& src) : _conjuncts(src._conjuncts) {}
    Conditions(Conditions&& src) : _conjuncts(std::move(src._conjuncts)) {}
    Conditions(size_t site, size_t choice) {
      add(site, choice);
      assert(_conjuncts.size() == 1);
      assert(!_conjuncts[0].empty());
    }

    iterator begin() { return _conjuncts.begin(); }
    iterator end() { return _conjuncts.end(); }
    const_iterator begin() const { return _conjuncts.begin(); }
    const_iterator end() const { return _conjuncts.end(); }

    /** Empty means there are no conditions. */
    bool empty() const {
      return _conjuncts.empty();
    }

    /** Number of conjuncts. */
    size_t numConjuncts() const { return _conjuncts.size(); }

    iterator find(size_t site) {
      auto it = findInsertionPoint(site);
      return it != _conjuncts.end() && it->site() == site ? it : _conjuncts.end();
    }

    /** Add elements to the set. */
    bool add(size_t site, size_t choice) {
      auto it = findInsertionPoint(site);
      if (it != _conjuncts.end() && it->site() == site) {
        return it->add(choice);
      } else {
        _conjuncts.insert(it, Conjunct(site, choice));
        return true;
      }
    }

    /** Compute the conjunction of this set of conditions and another. */
    void conjoinWith(const Conditions& src) {
      for (auto& it : src) {
        auto dit = findInsertionPoint(it.site());
        if (dit != _conjuncts.end() && dit->site() == it.site()) {
          dit->intersectWith(it);
        } else {
          _conjuncts.insert(dit, it);
        }
      }
    }

    /** True if conditions are equal. */
    bool equal(const Conditions& other) const {
      return std::equal(
          _conjuncts.begin(), _conjuncts.end(),
          other._conjuncts.begin(), other._conjuncts.end());
    }

    bool operator==(const Conditions& other) const {
      return equal(other);
    }

    bool operator!=(const Conditions& other) const {
      return !equal(other);
    }

    /** True if this 'superset' includes all the cases of this set. Note that while adding more
        terms to a conjunct makes it *larger*, adding more conjuncts makes it *smaller*.
        That is because adding more conjuncts means it applies to fewer cases.
    */
    bool isSubset(const Conditions& superset) const {
      auto first1 = begin(), last1 = end();
      auto first2 = superset.begin(), last2 = superset.end();
      for (; first2 != last2; ++first1) {
        if (first1 == last1) {
          return false;
        } else if (first2->site() < first1->site()) {
          return false;
        } else if (first2->site() == first1->site() && !first1->isSubset(*first2)) {
          return false;
        }
        if (!(first1->site() < first2->site())) {
          ++first2;
        }
      }
      return true;
    }

    /** Set operations. */
    Conditions& operator&=(const Conditions& src) {
      conjoinWith(src);
      return *this;
    }

    friend Conditions operator&(const Conditions& a, const Conditions& b) {
      Conditions result(a);
      result.conjoinWith(b);
      return result;
    }

    Conditions& operator=(const Conditions& src) {
      _conjuncts = src._conjuncts;
      return *this;
    }

    Conditions& operator=(Conditions&& src) {
      _conjuncts = std::move(src._conjuncts);
      return *this;
    }

  private:
    Conjuncts _conjuncts;

    iterator findInsertionPoint(size_t site) {
      return std::lower_bound(
          _conjuncts.begin(),
          _conjuncts.end(), site,
          [](const Conjunct& l, const Conjunct& r) {
            return l.site() < r.site();
          });
    }
  };

  // How to print a node kind.
  inline ::std::ostream& operator<<(::std::ostream& os, const Conditions& c) {
    os << "{";
    auto s2 = "";
    for (auto cj : c) {
      os << s2 << cj.site() << ":[";
      auto sep = "";
      for (auto choice : cj) {
        os << sep << choice;
        sep = " or ";
      }
      os << "]";
      s2 = " and ";
    }
    os << "}";
    return os;
  }
}

#endif
