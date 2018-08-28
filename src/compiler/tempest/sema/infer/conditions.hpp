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
      a particular type constraint to be applicable.

      The individual terms of the conditions represent the choice of which overload
      candidate is chosen for a particular call site. During type inference, the set of
      overloads may initially be ambiguous, which means that multiple overloads may be
      chosen for a single site, but eventually those choices must be narrowed to
      a single overload per site.

      Note all overload sites will be included in a given condition set, since not every
      overload site affects whether a given type constraint is applicable.

      This class supports logical operations on condition sets. Adding a conjunct to a set
      makes it more restrictive, while adding choices within a conjunct makes it less
      restrictive.

      Because sets are typically very small (1 or 2 elements), we use a SmallVector to store
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
      bool includes(const Conjunct& subset) const {
        for (auto choice : subset._choices) {
          if (!contains(choice)) {
            return false;
          }
        }
        return true;
      }

      /** True if this set is a subset of `superset`. */
      bool isSubset(const Conjunct& superset) const {
        for (auto choice : _choices) {
          if (!superset.contains(choice)) {
            return false;
          }
        }
        return true;
      }

      /** Add elements to the set. Maintains sorted order. */
      bool add(size_t choice) {
        if (!contains(choice)) {
          _choices.insert(std::upper_bound(_choices.begin(), _choices.end(), choice), choice);
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

    iterator findSite(size_t site) {
      return std::find_if(
          _conjuncts.begin(),
          _conjuncts.end(),
          [site](const Conjunct& c) { return c.site() == site; });
    }

    /** Add elements to the set. */
    bool add(size_t site, size_t choice) {
      auto it = findSite(site);
      if (it != _conjuncts.end()) {
        return it->add(choice);
      } else {
        _conjuncts.push_back(Conjunct(site, choice));
        return true;
      }
    }

    /** Compute the conjunction of this set of conditions and another. */
    void conjoinWith(const Conditions& src) {
      for (auto& it : src) {
        auto dst = findSite(it.site());
        if (dst == _conjuncts.end()) {
          _conjuncts.push_back(it);
        } else {
          dst->intersectWith(it);
        }
      }
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

  private:
    Conjuncts _conjuncts;
  };
}

#endif
