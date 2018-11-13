#ifndef TEMPEST_GEN_SYMBOLSTORE_HPP
#define TEMPEST_GEN_SYMBOLSTORE_HPP 1

#ifndef TEMPEST_GEN_OUTPUTSYM_HPP
  #include "tempest/gen/outputsym.hpp"
#endif

namespace tempest::gen {
  template<class A, class B>
  struct PairHash {
    inline std::size_t operator()(const std::pair<A, B>& value) const {
      std::size_t hash = std::hash<A>()(value.first);
      tempest::support::hash_combine(hash, std::hash<B>()(value.second));
      return hash;
    }
  };

  /** Contains all of the output symbols. */
  class SymbolStore {
  public:
    tempest::support::BumpPtrAllocator& alloc() { return _alloc; }

    FunctionSym* addFunction(FunctionDefn* function, const ArrayRef<const Type*>& typeArgs);
    ClassDescriptorSym* addClass(TypeDefn* clsDefn, const ArrayRef<const Type*>& typeArgs);
    InterfaceDescriptorSym* addInterface(TypeDefn* clsDefn, const ArrayRef<const Type*>& typeArgs);
    GlobalVarSym* addGlobalVar(ValueDefn* varDefn, const ArrayRef<const Type*>& typeArgs);
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
        std::pair<ClassDescriptorSym*, InterfaceDescriptorSym*>,
        ClassInterfaceTranslationSym*,
        PairHash<ClassDescriptorSym*, InterfaceDescriptorSym*>> _clsIfTrans;
    std::unordered_map<
        SpecializationKey<ValueDefn>,
        GlobalVarSym*,
        SpecializationKeyHash<ValueDefn>> _globals;
    std::vector<OutputSym*> _list;
  };
}

#endif
