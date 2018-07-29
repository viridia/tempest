#include "tempest/sema/graph/symboltable.hpp"
#include <assert.h>

namespace tempest::sema::graph {
  void SymbolTable::addMember(Member* m) {
    assert(m->kind >= Member::Kind::TYPE && m->kind < Member::Kind::COUNT);
    _entries[m->name()].push_back(m);
  }

  void SymbolTable::lookupName(const llvm::StringRef& name, std::vector<Member*>& result) const {
    EntryMap::const_iterator it = _entries.find(name);
    if (it != _entries.end()) {
      result.insert(result.end(), it->second.begin(), it->second.end());
    }
  }

  void SymbolTable::forAllNames(const NameCallback& nameFn) const {
    for (auto&& v : _entries) {
      nameFn(v.first());
    }
  }

  // void SymbolTable::describe(std::ostream& strm) const {
  //   if (_owner) {
  //     if (_description.size() > 0) {
  //       strm << _description << " scope for " << _owner->name();
  //     } else {
  //       strm << "scope for " << _owner->name();
  //     }
  //   } else if (_description.size() > 0) {
  //     strm << _description << " scope";
  //   } else {
  //     strm << "scope containing " << _entries.size() << " members";
  //   }
  // }

  // void SymbolTable::validate() const {
  //   for (EntryMap::value_type v : _entries) {
  //     for (auto m : v.second) {
  //       assert(m->kind() >= Member::Kind::TYPE && m->kind() <= Member::Kind::TUPLE_MEMBER);
  //     }
  //   }
  // }
}
