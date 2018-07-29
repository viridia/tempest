#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

namespace tempest::intrinsic {
  using namespace tempest::sema::graph;
  using tempest::source::Location;

  /** Class to contain all of the various intrinsic definitions. */
  IntrinsicDefns::IntrinsicDefns() {
    // Built-in classes
    objectClass = makeTypeDefn(Type::Kind::CLASS, "Object");
    objectClass->setIntrinsic(IntrinsicType::OBJECT_CLASS);

    // std::unique_ptr<TypeDefn*> objectClass;
    // std::unique_ptr<TypeDefn*> throwableClass;
    // std::unique_ptr<TypeDefn*> classDescriptorStruct;
    // std::unique_ptr<TypeDefn*> interfaceDescriptorStruct;
    // std::unique_ptr<TypeDefn*> iterableTrait;
    // std::unique_ptr<TypeDefn*> iteratorTrait;

    // Built-in traits
    // additionTrait = makeTypeDefn(Type::Kind::CLASS, "Addition");
    // additionTrait->setIntrinsic(IntrinsicType::ADDITION_TRAIT);
  }

  std::unique_ptr<TypeDefn> IntrinsicDefns::makeTypeDefn(Type::Kind kind, llvm::StringRef name) {
    auto td = std::make_unique<TypeDefn>(Location(), name);
    auto ty = new UserDefinedType(kind, td.get());
    td->setType(ty);
    return td;
  }

  ValueDefn* IntrinsicDefns::addValueDefn(
      std::unique_ptr<TypeDefn>& td,
      Member::Kind kind,
      llvm::StringRef name) {
    auto vd = new ValueDefn(kind, Location(), name, td.get());
    td->members().push_back(vd);
    td->memberScope()->addMember(vd);
    return vd;
  }

  IntrinsicDefns* IntrinsicDefns::get() {
    if (instance == nullptr) {
      instance = new IntrinsicDefns();
    }
    return instance;
  }

  IntrinsicDefns* IntrinsicDefns::instance = nullptr;
}
