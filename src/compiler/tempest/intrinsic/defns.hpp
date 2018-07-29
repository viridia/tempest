#ifndef TEMPEST_INTRINSIC_DEFNS_HPP
#define TEMPEST_INTRINSIC_DEFNS_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

namespace tempest::intrinsic {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::ValueDefn;

  /** Class to contain all of the various intrinsic definitions. */
  class IntrinsicDefns {
  public:
    IntrinsicDefns();

    std::unique_ptr<TypeDefn> objectClass;
    std::unique_ptr<TypeDefn> throwableClass;
    // std::unique_ptr<TypeDefn*> classDescriptorStruct;
    // std::unique_ptr<TypeDefn*> interfaceDescriptorStruct;
    // std::unique_ptr<TypeDefn*> iterableTrait;
    // std::unique_ptr<TypeDefn*> iteratorTrait;

    // Operator traits
    std::unique_ptr<TypeDefn> additionTrait;
    std::unique_ptr<TypeDefn> subtractionTrait;
    std::unique_ptr<TypeDefn> multiplicationTrait;
    std::unique_ptr<TypeDefn> divisionTrait;
    std::unique_ptr<TypeDefn> modulusTrait;

    // Singleton getter.
    static IntrinsicDefns* get();

  private:
    std::unique_ptr<TypeDefn> makeTypeDefn(Type::Kind kind, llvm::StringRef name);
    ValueDefn* addValueDefn(
        std::unique_ptr<TypeDefn>& td,
        Member::Kind kind,
        llvm::StringRef name);

    static IntrinsicDefns* instance;
  };
}

#endif
