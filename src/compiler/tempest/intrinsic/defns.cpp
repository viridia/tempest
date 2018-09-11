#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

namespace tempest::intrinsic {
  using namespace tempest::sema::graph;
  using tempest::source::Location;

  /** Class to contain all of the various intrinsic definitions. */
  IntrinsicDefns::IntrinsicDefns() {
    // Built-in scope
    builtinScope = std::make_unique<SymbolTable>();

    // Built-in classes
    objectClass = makeTypeDefn(Type::Kind::CLASS, "Object");
    objectClass->setIntrinsic(IntrinsicType::OBJECT_CLASS);

    // std::unique_ptr<TypeDefn*> objectClass;
    // std::unique_ptr<TypeDefn*> throwableClass;
    // std::unique_ptr<TypeDefn*> classDescriptorStruct;
    // std::unique_ptr<TypeDefn*> interfaceDescriptorStruct;
    // std::unique_ptr<TypeDefn*> iterableTrait;
    // std::unique_ptr<TypeDefn*> iteratorTrait;

    intOps.add = makeInfixOp("infixAdd", &IntegerType::I64);
    uintOps.add = makeInfixOp("infixAdd", &IntegerType::U64);
    floatOps.add = makeInfixOp("infixAdd", &FloatType::F64);

    intOps.subtract = makeInfixOp("infixSubtract", &IntegerType::I64);
    uintOps.subtract = makeInfixOp("infixSubtract", &IntegerType::U64);
    floatOps.subtract = makeInfixOp("infixSubtract", &FloatType::F64);

    intOps.multiply = makeInfixOp("infixMultiply", &IntegerType::I64);
    uintOps.multiply = makeInfixOp("infixMultiply", &IntegerType::U64);
    floatOps.multiply = makeInfixOp("infixMultiply", &FloatType::F64);

    intOps.divide = makeInfixOp("infixDivide", &IntegerType::I64);
    uintOps.divide = makeInfixOp("infixDivide", &IntegerType::U64);
    floatOps.divide = makeInfixOp("infixDivide", &FloatType::F64);

    intOps.remainder = makeInfixOp("infixRemainder", &IntegerType::I64);
    uintOps.remainder = makeInfixOp("infixRemainder", &IntegerType::U64);
    floatOps.remainder = makeInfixOp("infixRemainder", &FloatType::F64);

    // Built-in traits
    // additionTrait = makeTypeDefn(Type::Kind::CLASS, "Addition");
    // additionTrait->setIntrinsic(IntrinsicType::ADDITION_TRAIT);
  }

  std::unique_ptr<TypeDefn> IntrinsicDefns::makeTypeDefn(Type::Kind kind, llvm::StringRef name) {
    auto td = std::make_unique<TypeDefn>(Location(), name);
    auto ty = new UserDefinedType(kind, td.get());
    td->setType(ty);
    builtinScope->addMember(td.get());
    return td;
  }

  std::unique_ptr<FunctionDefn> IntrinsicDefns::makeInfixOp(llvm::StringRef name, Type* argType) {
    auto fd = std::make_unique<FunctionDefn>(Location(), name);
    fd->setIntrinsic(true);
    auto T = new (_types.alloc()) TypeParameter(Location(), "T");
    T->setTypeVar(new (_types.alloc()) TypeVar(T));
    SmallVector<Type*, 1> subtypeConstraints;
    subtypeConstraints.push_back(argType);
    SmallVector<Type*, 2> paramTypes;
    T->setSubtypeConstraints(_types.alloc().copyOf(subtypeConstraints));
    auto param0 = new (_types.alloc()) ParameterDefn(Location(), "a0", fd.get());
    param0->setType(T->typeVar());
    fd->params().push_back(param0);
    paramTypes.push_back(T->typeVar());
    auto param1 = new (_types.alloc()) ParameterDefn(Location(), "a1", fd.get());
    param1->setType(T->typeVar());
    fd->params().push_back(param1);
    paramTypes.push_back(T->typeVar());
    fd->setType(_types.createFunctionType(T->typeVar(), paramTypes, false));
    fd->typeParams().push_back(T);
    fd->allTypeParams().push_back(T);
    builtinScope->addMember(fd.get());
    return fd;
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
