#ifndef TEMPEST_COLLECTION_AGENDA_H
#define TEMPEST_COLLECTION_AGENDA_H 1

#include <unordered_set>
#include <vector>

namespace templest::collection {

  template<class T, class Hasher = std::hash<T> >
  class Agenda {
  public:
    typedef std::vector<T>::iterator iterator;
    typedef std::vector<T>::const_iterator const_iterator;
    typedef T& reference;
    typedef T& const_reference;

    /** Add an item to the agenda if it isn't there already. Returns true if the item was already
        in the agenda. */
    bool add(T& item) {
      auto it = _contains.find(item);
      if (it != _contains.end()) {
        return true;
      }
      _order.push_back(item);
      _contains.insert(item);
      return false;
    }

    /** Return the number of items in the aganda. */
    size_t size() const { return _order.size(); }

    /** True if the agenda is empty. */
    bool empty() const { return _order.empty(); }

    /** Return the index of the next item to work on. */
    size_t position() const { return _position; }

    /** Return true if there are no more items to work on. */
    bool done() const { return _position >= _order.size(); }

    /** Return the next item in the agent. */
    T& next() const { return _order[_position++]; }

    /** Read-only random access. */
    char operator[](int index) const {
      return _order[index];
    }

  private:
    std::unordered_set<T> _contains;
    std::vector<T> _order;
    size_t _position = 0;
  };

}

#endif
