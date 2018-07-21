#ifndef TEMPEST_SEMA_GRAPH_ENV_H
#define TEMPEST_SEMA_GRAPH_ENV_H 1

#ifndef TEMPEST_CONFIG_H
  #include "config.h"
#endif

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <utility>

namespace tempest::sema::graph {
  class Type;
  class TypeVar;

  /** A set of substitutions to be applied to a type expression. */
  class Env {
  public:
    typedef const TypeVar* key_type;
    typedef Type* value_type;
    typedef std::pair<key_type, value_type> element_type;
    typedef element_type* iterator;
    typedef const element_type* const_iterator;

    Env() : _data(nullptr), _size(0) {}

    /** The number of elements in the set. */
    size_t size() const { return _size; }

    /** Pointer to the element data. */
    const element_type* data() const { return _data; }

    /** Find an element in the set. */
    const_iterator find(const key_type& key) const {
      for (size_t i = 0; i < _size; ++i) {
        if (_data[i].first == key) {
          return &_data[i];
        }
      }
      return &_data[_size];
    }

    /** Compare two envs for equality. */
    bool operator==(const Env& other) const {
      if (other.size() != size()) {
        return false;
      }
      for (auto entry : *this) {
        auto it = other.find(entry.first);
        if (it == other.end() || it->second != entry.second) {
          return false;
        }
      }
      return true;
    }

    /** Compare two envs for inequality. */
    bool operator!=(const Env& other) const {
      return !(*this == other);
    }

    /** Iterators. Items will be interated in order of insertion. */
    const_iterator begin() const { return _data; }
    const_iterator end() const { return _data + _size; }

  protected:
    // Constructor that allows the map to be pre-populated.
    Env(element_type* data, size_t size) : _data(data), _size(size) {}

    iterator findEntry(const key_type& key) const {
      for (size_t i = 0; i < _size; ++i) {
        if (_data[i].first == key) {
          return &_data[i];
        }
      }
      return &_data[_size];
    }

    element_type* _data;
    std::size_t _size;
  };

  /** An immutable environment. */
  class ImmutableEnv : public Env {
  public:
    ImmutableEnv() : Env(nullptr, 0) {}
    ImmutableEnv(element_type* data, size_t size) : Env(data, size) {}
    // I hate using const_cast, but in this case we know we are never going to mutate it.
    ImmutableEnv(const element_type* data, size_t size)
      : Env(const_cast<element_type*>(data), size) {}
  //   ImmutableMap(const ArrayRef<element_type>& src) : Env(src.begin(), src.size()) {}
    ImmutableEnv(const ImmutableEnv& src) : Env(src._data, src.size()) {}

    // ImmutableEnv& operator=(const ImmutableEnv& src) {
    //   this->_data = src._data;
    //   this->_size = src._size;
    //   return *this;
    // }

    /** Array access operator. */
    const value_type& operator[](const key_type& key) const {
      const_iterator it = findEntry(key);
      if (it != this->end()) {
        return it->second;
      }
      assert(false && "Entry not found");
    }
  };

  /** An abstract env that can allocate additional capacity as elements are inserted. Elements
      are always kept in insertion order. */
  class SmallEnvBase : public Env {
  public:
    SmallEnvBase() : _alloc(nullptr), _capacity(0) {}
    ~SmallEnvBase() {
      if (_alloc != nullptr) {
        delete _alloc;
      }
    }

    /** Insert a key and value into the map. */
    iterator insert(const element_type& element) {
      iterator it = this->findEntry(element.first);
      if (it == this->end()) {
        return append(element);
      } else {
        it->second = element.second;
        return it;
      }
    }

    /** Insert a key and value into the map. */
    iterator insert(const key_type& key, const value_type& value) {
      return insert(std::pair<key_type, value_type>(key, value));
    }

    /** Insert a range of key/value pairs into the map. */
    template<class Iter>
    void insert(Iter first, Iter last) {
      while (first != last) {
        insert(*first++);
      }
    }

    /** Array access operator (mutable version). */
    value_type& operator[](const key_type& key) {
      iterator it = this->findEntry(key);
      if (it != this->end()) {
        return it->second;
      }
      it = append(std::pair<key_type, value_type>(key, value_type()));
      return it->second;
    }

    /** Array access operator (const version). */
    const value_type& operator[](const key_type& key) const {
      iterator it = this->findEntry(key);
      if (it != this->end()) {
        return it->second;
      }
      assert(false && "Entry not found");
    }

  protected:
    SmallEnvBase(element_type* data, size_t capacity)
      : Env(data, 0)
      , _alloc(nullptr)
      , _capacity(capacity)
    {}

    iterator append(const element_type& element) {
      if (this->_size + 1 >= _capacity) {
        resize();
      }
      iterator it = &this->_data[this->_size++];
      *it = element;
      return it;
    }

    void resize() {
      _capacity = this->_size * 2;
      // TODO: This calls constructors, which we should avoid. Should use uninitialized_copy here.
      element_type* data = new element_type[_capacity];
      std::copy(this->_data, &this->_data[this->_size], data);
      if (_alloc != nullptr) {
        delete _alloc;
      }
      this->_data = _alloc = data;
    }

    element_type* _alloc;
    size_t _capacity;
  };

  /** An env class optimized for small numbers of elements. Envs less than size N will not allocate
      any memory on the heap. Items are kept in insertion order. Does not require a hash
      function. */
  template <int N>
  class SmallEnv : public SmallEnvBase {
  public:
    SmallEnv() : SmallEnvBase(_elements, N) {}
    SmallEnv(const Env& src) : SmallEnvBase(_elements, N) {
      for (auto el : src) {
        append(el);
      }
    }

  private:
    element_type _elements[N];
  };

  /** A concatenation of environments. This represents the operation of applying a sequence
      of substitutions to a type expression. Note that this does not take ownership of
      the previous environment.

      Typical use case:

        class A[T] {
          let m: Array[T];
        }
        let a: A[i32]; // What's the type of a.m?

      Env { T => i32 }
      Env { ElementType => T }
  */
  class EnvChain {
  public:
    const Env& env;
    const EnvChain* const prev;

    EnvChain(const Env& e) : env(e), prev(&NIL) {}
    EnvChain(const Env& e, const EnvChain& p) : env(e), prev(&p) {}
    EnvChain(const EnvChain& e) : env(e.env), prev(e.prev) {}

    /** Returns true if this is the sentinel node at the end of the linked list. */
    bool isNil() const { return this == &NIL; }

    /** Given a type variable, return the concrete type it maps to. */
    // Type* get(const TypeVar* tv);

    /** Sentinel node to indicate the end of the linked list of Envs. */
    static EnvChain NIL;
  };
}

#endif
