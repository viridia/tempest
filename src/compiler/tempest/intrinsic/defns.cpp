#include "tempest/intrinsic/builder.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

namespace tempest::intrinsic {
  using namespace tempest::sema::graph;
  using tempest::source::Location;

  static ExternalIntrinsicFn intrinsicFns[] = {
    { "tempest.core.memory.copy", IntrinsicFn::MEMORY_COPY },
    { "tempest.core.memory.move", IntrinsicFn::MEMORY_MOVE },
    { "tempest.core.memory.fill", IntrinsicFn::MEMORY_FILL },
  };

  static ExternalIntrinsicType intrinsicTypes[] = {
    { "tempest.core.memory.Address", IntrinsicType::ADDRESS_TYPE },
    { "tempest.core.iterate.Iterable", IntrinsicType::ITERABLE_TYPE },
    { "tempest.core.iterate.Iterator", IntrinsicType::ITERATOR_TYPE },
  };

  static void qualName(std::string& out, Member* m) {
    if (m->definedIn()) {
      qualName(out, m->definedIn());
      out.append(".");
    }
    out.append(m->name().begin(), m->name().end());
  }

  /** Class to contain all of the various intrinsic definitions. */
  IntrinsicDefns::IntrinsicDefns() {
    // Built-in scope
    builtinScope = std::make_unique<SymbolTable>();

    // Built-in classes
    objectClass = makeTypeDefn(Type::Kind::CLASS, "Object");
    objectClass->setIntrinsic(IntrinsicType::OBJECT_CLASS);

    // Object constructor
    BuiltinMethodBuilder(_types, objectClass, "new")
        .intrinsic(IntrinsicFn::OBJECT_CTOR)
        .constructor()
        .build();

    // Base class of flex-alloc containers
    auto flexAllocT = new (_types.alloc()) TypeParameter(Location(), "El");
    flexAllocT->setTypeVar(new (_types.alloc()) TypeVar(flexAllocT));

    flexAllocClass = makeTypeDefn(Type::Kind::CLASS, "FlexAlloc");
    flexAllocClass->setIntrinsic(IntrinsicType::FLEXALLOC_CLASS);
    flexAllocClass->extends().push_back(objectClass.get());
    flexAllocClass->typeParams().push_back(flexAllocT);
    flexAllocClass->allTypeParams().push_back(flexAllocT);

    BuiltinMethodBuilder(_types, flexAllocClass, "new")
        .addParam("size", &IntegerType::I64)
        .intrinsic(IntrinsicFn::OBJECT_CTOR)
        .constructor()
        .build();

    // FlexAlloc __alloc
    auto AllocT = new (_types.alloc()) TypeParameter(Location(), "T");
    AllocT->setTypeVar(new (_types.alloc()) TypeVar(AllocT));

    BuiltinMethodBuilder(_types, flexAllocClass, "__alloc")
        .addTypeParam(AllocT)
        .addParam("size", &IntegerType::I64)
        .returnType(AllocT->typeVar())
        .intrinsic(IntrinsicFn::FLEX_ALLOC)
        .setStatic()
        .build();

    // std::unique_ptr<TypeDefn*> throwableClass;
    // std::unique_ptr<TypeDefn*> classDescriptorStruct;
    // std::unique_ptr<TypeDefn*> interfaceDescriptorStruct;
    // std::unique_ptr<TypeDefn*> iterableTrait;
    // std::unique_ptr<TypeDefn*> iteratorTrait;

    intOps.add = makeInfixOp("infixAdd", &IntegerType::I64, IntrinsicFn::ADD);
    uintOps.add = makeInfixOp("infixAdd", &IntegerType::U64, IntrinsicFn::ADD);
    floatOps.add = makeInfixOp("infixAdd", &FloatType::F64, IntrinsicFn::ADD);

    intOps.subtract = makeInfixOp("infixSubtract", &IntegerType::I64, IntrinsicFn::SUBTRACT);
    uintOps.subtract = makeInfixOp("infixSubtract", &IntegerType::U64, IntrinsicFn::SUBTRACT);
    floatOps.subtract = makeInfixOp("infixSubtract", &FloatType::F64, IntrinsicFn::SUBTRACT);

    intOps.multiply = makeInfixOp("infixMultiply", &IntegerType::I64, IntrinsicFn::MULTIPLY);
    uintOps.multiply = makeInfixOp("infixMultiply", &IntegerType::U64, IntrinsicFn::MULTIPLY);
    floatOps.multiply = makeInfixOp("infixMultiply", &FloatType::F64, IntrinsicFn::MULTIPLY);

    intOps.divide = makeInfixOp("infixDivide", &IntegerType::I64, IntrinsicFn::DIVIDE);
    uintOps.divide = makeInfixOp("infixDivide", &IntegerType::U64, IntrinsicFn::DIVIDE);
    floatOps.divide = makeInfixOp("infixDivide", &FloatType::F64, IntrinsicFn::DIVIDE);

    intOps.remainder = makeInfixOp("infixRemainder", &IntegerType::I64, IntrinsicFn::REMAINDER);
    uintOps.remainder = makeInfixOp("infixRemainder", &IntegerType::U64, IntrinsicFn::REMAINDER);
    floatOps.remainder = makeInfixOp("infixRemainder", &FloatType::F64, IntrinsicFn::REMAINDER);

    intOps.lsh = makeInfixOp("infixLeftShift", &IntegerType::I64, IntrinsicFn::LSHIFT);
    uintOps.lsh = makeInfixOp("infixLeftShift", &IntegerType::U64, IntrinsicFn::LSHIFT);

    intOps.rsh = makeInfixOp("infixRightShift", &IntegerType::I64, IntrinsicFn::RSHIFT);
    uintOps.rsh = makeInfixOp("infixRightShift", &IntegerType::U64, IntrinsicFn::RSHIFT);

    intOps.bitOr = makeInfixOp("infixBitOr", &IntegerType::I64, IntrinsicFn::BIT_OR);
    uintOps.bitOr = makeInfixOp("infixBitOr", &IntegerType::U64, IntrinsicFn::BIT_OR);

    intOps.bitAnd = makeInfixOp("infixBitAnd", &IntegerType::I64, IntrinsicFn::BIT_AND);
    uintOps.bitAnd = makeInfixOp("infixBitAnd", &IntegerType::U64, IntrinsicFn::BIT_AND);

    intOps.bitXor = makeInfixOp("infixBitXOr", &IntegerType::I64, IntrinsicFn::BIT_XOR);
    uintOps.bitXor = makeInfixOp("infixBitXOr", &IntegerType::U64, IntrinsicFn::BIT_XOR);

    eq = makeRelationalOp("isEqualTo", nullptr, IntrinsicFn::EQ);

    intOps.lt = makeRelationalOp("isLessThan", &IntegerType::I64, IntrinsicFn::LT);
    uintOps.lt = makeRelationalOp("isLessThan", &IntegerType::U64, IntrinsicFn::LT);
    floatOps.lt = makeRelationalOp("isLessThan", &FloatType::F64, IntrinsicFn::LT);

    intOps.le = makeRelationalOp("isLessThanOrEqual", &IntegerType::I64, IntrinsicFn::LE);
    uintOps.le = makeRelationalOp("isLessThanOrEqual", &IntegerType::U64, IntrinsicFn::LE);
    floatOps.le = makeRelationalOp("isLessThanOrEqual", &FloatType::F64, IntrinsicFn::LE);

    intOps.uminus = makeUnaryOp("unaryNegation", &IntegerType::I64, IntrinsicFn::NEGATE);
    uintOps.uminus = makeUnaryOp("unaryNegation", &IntegerType::U64, IntrinsicFn::NEGATE);
    floatOps.uminus = makeUnaryOp("unaryNegation", &FloatType::F64, IntrinsicFn::NEGATE);

    intOps.comp = makeUnaryOp("unaryComplement", &IntegerType::I64, IntrinsicFn::COMPLEMENT);
    uintOps.comp = makeUnaryOp("unaryComplement", &IntegerType::U64, IntrinsicFn::COMPLEMENT);
  }

  bool IntrinsicDefns::registerExternal(Defn* d) {
    std::string qname;
    qname.reserve(64);
    qualName(qname, d);
    if (auto fd = dyn_cast<FunctionDefn>(d)) {
      auto aref = ArrayRef<ExternalIntrinsicFn>(intrinsicFns);
      auto it = std::find_if(aref.begin(), aref.end(), [&qname](auto& ifn) {
        return ifn.name == qname;
      });
      if (it != aref.end()) {
        fd->setIntrinsic(it->intrinsic);
        return true;
      }
      return false;
    } else if (auto td = dyn_cast<TypeDefn>(d)) {
      auto aref = ArrayRef<ExternalIntrinsicType>(intrinsicTypes);
      auto it = std::find_if(aref.begin(), aref.end(), [&qname](auto& ifn) {
        return ifn.name == qname;
      });
      if (it != aref.end()) {
        td->setIntrinsic(it->intrinsic);
        switch (it->intrinsic) {
          case IntrinsicType::ADDRESS_TYPE:
            addressType = td;
            break;
          case IntrinsicType::ITERABLE_TYPE:
            iterableType = td;
            break;
          case IntrinsicType::ITERATOR_TYPE:
            iteratorType = td;
            break;
          default:
            break;
        }
        return true;
      }
      return false;
    } else {
      return false;
    }
  }

  std::unique_ptr<TypeDefn> IntrinsicDefns::makeTypeDefn(Type::Kind kind, llvm::StringRef name) {
    auto td = std::make_unique<TypeDefn>(Location(), name);
    auto ty = new (_types.alloc()) UserDefinedType(kind, td.get());
    td->setType(ty);
    builtinScope->addMember(td.get());
    return td;
  }

  std::unique_ptr<FunctionDefn> IntrinsicDefns::makeInfixOp(
      llvm::StringRef name, Type* argType, IntrinsicFn intrinsic) {
    auto fd = std::make_unique<FunctionDefn>(Location(), name);
    fd->setIntrinsic(intrinsic);
    auto T = new (_types.alloc()) TypeParameter(Location(), "T");
    T->setTypeVar(new (_types.alloc()) TypeVar(T));
    SmallVector<Type*, 1> subtypeConstraints;
    subtypeConstraints.push_back(argType);
    SmallVector<Type*, 2> paramTypes;
    T->setSubtypeConstraints(_types.alloc().copyOf(subtypeConstraints));
    auto param0 = new (_types.alloc()) ParameterDefn(Location(), "a0", fd.get(), T->typeVar());
    fd->params().push_back(param0);
    paramTypes.push_back(T->typeVar());
    auto param1 = new (_types.alloc()) ParameterDefn(Location(), "a1", fd.get(), T->typeVar());
    fd->params().push_back(param1);
    paramTypes.push_back(T->typeVar());
    fd->setType(_types.createFunctionType(T->typeVar(), paramTypes, false));
    fd->typeParams().push_back(T);
    fd->allTypeParams().push_back(T);
    builtinScope->addMember(fd.get());
    return fd;
  }

  std::unique_ptr<FunctionDefn> IntrinsicDefns::makeRelationalOp(
      llvm::StringRef name, Type* argType, IntrinsicFn intrinsic) {
    auto fd = std::make_unique<FunctionDefn>(Location(), name);
    fd->setIntrinsic(intrinsic);
    auto T = new (_types.alloc()) TypeParameter(Location(), "T");
    T->setTypeVar(new (_types.alloc()) TypeVar(T));
    if (argType) {
      SmallVector<Type*, 1> subtypeConstraints;
      subtypeConstraints.push_back(argType);
      T->setSubtypeConstraints(_types.alloc().copyOf(subtypeConstraints));
    }
    SmallVector<Type*, 2> paramTypes;
    auto param0 = new (_types.alloc()) ParameterDefn(Location(), "a0", fd.get(), T->typeVar());
    fd->params().push_back(param0);
    paramTypes.push_back(T->typeVar());
    auto param1 = new (_types.alloc()) ParameterDefn(Location(), "a1", fd.get(), T->typeVar());
    fd->params().push_back(param1);
    paramTypes.push_back(T->typeVar());
    fd->setType(_types.createFunctionType(&BooleanType::BOOL, paramTypes, false));
    fd->typeParams().push_back(T);
    fd->allTypeParams().push_back(T);
    builtinScope->addMember(fd.get());
    return fd;
  }

  std::unique_ptr<FunctionDefn> IntrinsicDefns::makeUnaryOp(
      llvm::StringRef name, Type* argType, IntrinsicFn intrinsic) {
    auto fd = std::make_unique<FunctionDefn>(Location(), name);
    fd->setIntrinsic(intrinsic);
    auto T = new (_types.alloc()) TypeParameter(Location(), "T");
    T->setTypeVar(new (_types.alloc()) TypeVar(T));
    SmallVector<Type*, 1> subtypeConstraints;
    subtypeConstraints.push_back(argType);
    SmallVector<Type*, 2> paramTypes;
    T->setSubtypeConstraints(_types.alloc().copyOf(subtypeConstraints));
    auto param0 = new (_types.alloc()) ParameterDefn(Location(), "arg", fd.get());
    param0->setType(T->typeVar());
    fd->params().push_back(param0);
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
