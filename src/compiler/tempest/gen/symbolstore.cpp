#include "tempest/gen/symbolstore.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::gen {
  FunctionSym* SymbolStore::addFunction(
      FunctionDefn* function, const ArrayRef<const Type*>& typeArgs) {
    assert(function->allTypeParams().size() == typeArgs.size());
    assert(function->body());
    SpecializationKey key(function, typeArgs);
    auto it = _functions.find(key);
    if (it != _functions.end()) {
      return it->second;
    }

    auto fs = new (_alloc) FunctionSym(function, _alloc.copyOf(typeArgs));
    SpecializationKey newKey(function, fs->typeArgs);
    _functions[newKey] = fs;
    _list.push_back(fs);
    return fs;
  }

  ClassDescriptorSym* SymbolStore::addClass(
      TypeDefn* typeDefn, const ArrayRef<const Type*>& typeArgs) {
    assert(typeDefn->allTypeParams().size() == typeArgs.size());
    SpecializationKey key(typeDefn, typeArgs);
    auto it = _classes.find(key);
    if (it != _classes.end()) {
      return it->second;
    }

    auto cds = new (_alloc) ClassDescriptorSym(typeDefn, _alloc.copyOf(typeArgs));
    SpecializationKey newKey(typeDefn, cds->typeArgs);
    _classes[newKey] = cds;
    _list.push_back(cds);
    return cds;
  }

  InterfaceDescriptorSym* SymbolStore::addInterface(
      TypeDefn* typeDefn, const ArrayRef<const Type*>& typeArgs) {
    assert(typeDefn->allTypeParams().size() == typeArgs.size());
    SpecializationKey key(typeDefn, typeArgs);
    auto it = _interfaces.find(key);
    if (it != _interfaces.end()) {
      return it->second;
    }

    auto ids = new (_alloc) InterfaceDescriptorSym(typeDefn, _alloc.copyOf(typeArgs));
    SpecializationKey newKey(typeDefn, ids->typeArgs);
    _interfaces[newKey] = ids;
    _list.push_back(ids);
    return ids;
  }

  GlobalVarSym* SymbolStore::addGlobalVar(
      ValueDefn* varDefn, const ArrayRef<const Type*>& typeArgs) {
    SpecializationKey key(varDefn, typeArgs);
    auto it = _globals.find(key);
    if (it != _globals.end()) {
      return it->second;
    }

    auto gs = new (_alloc) GlobalVarSym(varDefn, _alloc.copyOf(typeArgs));
    SpecializationKey newKey(varDefn, gs->typeArgs);
    _globals[newKey] = gs;
    _list.push_back(gs);
    return gs;
  }

  ClassInterfaceTranslationSym* SymbolStore::addClassInterfaceTranslation(
      ClassDescriptorSym* cls,
      InterfaceDescriptorSym* iface) {
    auto key = std::make_pair(cls, iface);
    auto it = _clsIfTrans.find(key);
    if (it != _clsIfTrans.end()) {
      return it->second;
    }
    auto cit = new (_alloc) ClassInterfaceTranslationSym(cls, iface);
    _clsIfTrans[key] = cit;
    _list.push_back(cit);
    return cit;
  }

  FunctionSym* SymbolStore::findFunction(StringRef name) {
    auto it = std::find_if(_functions.begin(), _functions.end(), [name](auto& sym) {
      return sym.second->function->name() == name;
    });
    assert(it != _functions.end());
    return it->second;
  }

  ClassDescriptorSym* SymbolStore::findClass(StringRef name) {
    auto it = std::find_if(_classes.begin(), _classes.end(), [name](auto sym) {
      return sym.second->typeDefn->name() == name;
    });
    assert(it != _classes.end());
    return it->second;
  }

  InterfaceDescriptorSym* SymbolStore::findInterface(StringRef name) {
    auto it = std::find_if(_interfaces.begin(), _interfaces.end(), [name](auto sym) {
      return sym.second->typeDefn->name() == name;
    });
    assert(it != _interfaces.end());
    return it->second;
  }

  GlobalVarSym* SymbolStore::findGlobalVar(StringRef name) {
    auto it = std::find_if(_globals.begin(), _globals.end(), [name](auto sym) {
      return sym.second->varDefn->name() == name;
    });
    assert(it != _globals.end());
    return it->second;
  }

  ClassInterfaceTranslationSym* SymbolStore::findTranslation(
      ClassDescriptorSym* cls, InterfaceDescriptorSym* ifc) {
    auto it = std::find_if(_clsIfTrans.begin(), _clsIfTrans.end(), [cls, ifc](auto sym) {
      return sym.second->cls == cls && sym.second->iface == ifc;
    });
    assert(it != _clsIfTrans.end());
    return it->second;
  }
}
