#ifndef TEMPEST_GEN_SYMBOLSTORE_HPP
#define TEMPEST_GEN_SYMBOLSTORE_HPP 1

#ifndef TEMPEST_GEN_OUTPUTSYM_HPP
  #include "tempest/gen/outputsym.hpp"
#endif

namespace tempest::gen {
  /** Contains all of the output symbols. */
  class SymbolStore {
  public:
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    FunctionSym* addFunction(FunctionDefn* function, Env& env);
    ClassDescriptorSym* addClass(TypeDefn* clsDefn, Env& env);
    InterfaceDescriptorSym* addInterface(TypeDefn* clsDefn, Env& env);
    GlobalVarSym* addGlobalVar(ValueDefn* varDefn, Env& env);
    ClassInterfaceTranslationSym* addClassInterfaceTranslation(
        ClassDescriptorSym* cls,
        InterfaceDescriptorSym* iface);

    /** List of all output symbols in the order in which they were added. */
    std::vector<OutputSym*>& list() { return _list; }

  private:
    tempest::support::BumpPtrAllocator _alloc;

    std::unordered_map<
        SpecializationKey<FunctionDefn>,
        FunctionSym*,
        SpecializationKeyHash<FunctionDefn>> _functions;
    std::unordered_map<
        SpecializationKey<TypeDefn>,
        ClassDescriptorSym*,
        SpecializationKeyHash<TypeDefn>> _classes;
    std::unordered_map<
        SpecializationKey<TypeDefn>,
        InterfaceDescriptorSym*,
        SpecializationKeyHash<TypeDefn>> _interfaces;
    std::unordered_map<
        SpecializationKey<ValueDefn>,
        GlobalVarSym*,
        SpecializationKeyHash<ValueDefn>> _globals;
    std::vector<OutputSym*> _list;
  };
}

#endif