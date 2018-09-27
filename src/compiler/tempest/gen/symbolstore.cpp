#include "tempest/gen/symbolstore.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::gen {
  FunctionSym* SymbolStore::addFunction(FunctionDefn* function, Env& env) {
    assert(function->allTypeParams().size() == env.args.size());
    SpecializationKey key(function, env.args);
    auto it = _functions.find(key);
    if (it != _functions.end()) {
      return it->second;
    }

    auto fs = new (_alloc) FunctionSym(function, _alloc.copyOf(env.args));
    SpecializationKey newKey(function, fs->typeArgs);
    _functions[newKey] = fs;
    _list.push_back(fs);
    return fs;
  }

  ClassDescriptorSym* SymbolStore::addClass(TypeDefn* typeDefn, Env& env) {
    assert(typeDefn->allTypeParams().size() == env.args.size());
    SpecializationKey key(typeDefn, env.args);
    auto it = _classes.find(key);
    if (it != _classes.end()) {
      return it->second;
    }

    auto cds = new (_alloc) ClassDescriptorSym(typeDefn, _alloc.copyOf(env.args));
    SpecializationKey newKey(typeDefn, cds->typeArgs);
    _classes[newKey] = cds;
    _list.push_back(cds);
    return cds;
  }

  InterfaceDescriptorSym* SymbolStore::addInterface(TypeDefn* typeDefn, Env& env) {
    assert(typeDefn->allTypeParams().size() == env.args.size());
    SpecializationKey key(typeDefn, env.args);
    auto it = _interfaces.find(key);
    if (it != _interfaces.end()) {
      return it->second;
    }

    auto ids = new (_alloc) InterfaceDescriptorSym(typeDefn, _alloc.copyOf(env.args));
    SpecializationKey newKey(typeDefn, ids->typeArgs);
    _interfaces[newKey] = ids;
    _list.push_back(ids);
    return ids;
  }

  GlobalVarSym* SymbolStore::addGlobalVar(ValueDefn* varDefn, Env& env) {
    SpecializationKey key(varDefn, env.args);
    auto it = _globals.find(key);
    if (it != _globals.end()) {
      return it->second;
    }

    auto gs = new (_alloc) GlobalVarSym(varDefn, _alloc.copyOf(env.args));
    SpecializationKey newKey(varDefn, gs->typeArgs);
    _globals[newKey] = gs;
    _list.push_back(gs);
    return gs;
  }

  ClassInterfaceTranslationSym* SymbolStore::addClassInterfaceTranslation(
      ClassDescriptorSym* cls,
      InterfaceDescriptorSym* iface) {
    assert(false && "Implement");
  }
}
