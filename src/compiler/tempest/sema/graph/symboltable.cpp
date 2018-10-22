#include "tempest/sema/graph/symboltable.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::graph {
  void SymbolTable::addMember(Member* m) {
    assert(m->kind >= Member::Kind::TYPE && m->kind < Member::Kind::COUNT);
    _entries[m->name()].push_back(m);
  }

  void SymbolTable::addMember(const llvm::StringRef& name, Member* m) {
    assert(m->kind >= Member::Kind::TYPE && m->kind < Member::Kind::COUNT);
    _entries[name].push_back(m);
  }

  void SymbolTable::lookupName(const llvm::StringRef& name, NameLookupResultRef& result) const {
    EntryMap::const_iterator it = _entries.find(name);
    if (it != _entries.end()) {
      result.insert(result.end(), it->second.begin(), it->second.end());
    }
  }

  void SymbolTable::lookup(
      const llvm::StringRef& name, MemberLookupResultRef &result, Expr* stem) const {
    EntryMap::const_iterator it = _entries.find(name);
    if (it != _entries.end()) {
      for (auto member : it->second) {
        result.push_back({ member, stem });
      }
    }
  }

  bool SymbolTable::exists(const llvm::StringRef& name, source::Location &location) const {
    EntryMap::const_iterator it = _entries.find(name);
    if (it != _entries.end()) {
      auto defn = llvm::dyn_cast<Defn>(it->second.front());
      location = defn ? defn->location() : source::Location();
      return true;
    }
    return false;
  }

  void SymbolTable::forAllNames(const NameCallback& nameFn) const {
    for (auto&& v : _entries) {
      nameFn(v.first());
    }
  }

  void SymbolTable::forAllMembers(const MemberCallback& callback) const {
    for (auto&& v : _entries) {
      for (auto m : v.second) {
        callback(m, v.first());
      }
    }
  }
}
