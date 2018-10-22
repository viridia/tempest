#include "tempest/sema/graph/extensionmap.hpp"

namespace tempest::sema::graph {

  void ExtensionMap::add(TypeDefn* original, TypeDefn* extension) {
    _entries[original].push_back(extension);
  }

  ArrayRef<TypeDefn*> ExtensionMap::lookup(TypeDefn* key) const {
    EntryMap::const_iterator it = _entries.find(key);
    if (it != _entries.end()) {
      return it->second;
    }
    return ArrayRef<TypeDefn*>();
  }
}
