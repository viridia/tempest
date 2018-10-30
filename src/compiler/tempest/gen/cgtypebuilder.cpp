#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/cgtypebuilder.hpp"
#include "tempest/gen/linkagename.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <assert.h>

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::gen::getLinkageName;
  using tempest::intrinsic::IntrinsicDefns;
  using tempest::error::diag;

  llvm::Type* CGTypeBuilder::get(const Type* ty, ArrayRef<const Type*> typeArgs) {
    switch (ty->kind) {
      case Type::Kind::VOID: {
        return llvm::Type::getVoidTy(_context);
      }

      case Type::Kind::BOOLEAN: {
        return llvm::Type::getInt1Ty(_context);
      }

      case Type::Kind::INTEGER: {
        auto intTy = static_cast<const IntegerType*>(ty);
        return llvm::IntegerType::get(_context, intTy->bits());
      }

      case Type::Kind::FLOAT: {
        auto floatTy = static_cast<const FloatType*>(ty);
        return floatTy->bits() == 32
            ? llvm::Type::getFloatTy(_context)
            : llvm::Type::getDoubleTy(_context);
      }

      case Type::Kind::MODIFIED: {
        auto modTy = static_cast<const ModifiedType*>(ty);
        return get(modTy->base, typeArgs);
      }

      case Type::Kind::TUPLE: {
        auto tupleTy = static_cast<const TupleType*>(ty);
        llvm::SmallVector<llvm::Type*, 16> elts;
        for (auto member : tupleTy->members) {
          elts.push_back(getMemberType(member, typeArgs));
        }
        return llvm::StructType::create(elts);
      }

      case Type::Kind::UNION: {
        auto cgu = createUnion(static_cast<const UnionType*>(ty));
        return cgu->type;
      }

      case Type::Kind::CLASS: {
        auto cls = static_cast<const UserDefinedType*>(ty);
        auto td = cls->defn();
        SpecializationKey<TypeDefn> key(td, typeArgs);
        auto it = _typeDefns.find(key);
        if (it != _typeDefns.end()) {
          return it->second;
        }
        if (td->kind == Defn::Kind::SPECIALIZED) {
          assert(false && "Implement");
        } else {
          auto irType = createClass(cls, typeArgs);
          _typeDefns[key] = irType;
          return irType;
        }
      }

      // // Nominal types
      // STRUCT, INTERFACE, TRAIT, EXTENSION, ENUM,  // Composites
      // ALIAS,          // An alias for another type
      // TYPE_VAR,       // Reference to a template parameter

      // // Derived types
      // CONST,          // Const type modifier

      case Type::Kind::FUNCTION: {
        auto fty = static_cast<const FunctionType*>(ty);
        llvm::SmallVector<llvm::Type*, 16> paramTypes;
        // Context parameter for objects and closures.
        paramTypes.push_back(llvm::Type::getVoidTy(_context)->getPointerTo());
        for (auto param : fty->paramTypes) {
          // TODO: getParamType or getInternalParamType
          paramTypes.push_back(getMemberType(param, typeArgs));
        }
        auto returnType = getMemberType(fty->returnType, typeArgs);
        return llvm::FunctionType::get(returnType, paramTypes, false);
      }

      // TODO: We shouldn't need this; Everything should be expanded at this point.
      // case Type::Kind::SPECIALIZED: {
      //   auto specTy = static_cast<const SpecializedType*>(ty);
      //   result = get(specTy->base, EnvChain(specTy->env, env));
      //   break;
      // }

      default:
        assert(false && "Invalid type argument");
        return NULL;
    }
  }

  llvm::Type* CGTypeBuilder::getMemberType(const Type* ty, ArrayRef<const Type*> typeArgs) {
    llvm::Type* result = get(ty, typeArgs);
    if (ty->kind == Type::Kind::CLASS) {
      return llvm::PointerType::get(result, 1); // GC address space
    }
    return result;
  }

  llvm::Type* CGTypeBuilder::createClass(
      const UserDefinedType* cls,
      ArrayRef<const Type*> typeArgs) {
    auto td = cls->defn();
    llvm::SmallVector<llvm::Type*, 16> elts;

    // Qualified name
    std::string linkageName;
    linkageName.reserve(64);
    getLinkageName(linkageName, td, typeArgs);

    // Base class
    llvm::Type* flexAllocElementType = nullptr;
    if (td->intrinsic() != IntrinsicType::NONE) {
      switch (td->intrinsic()) {
        case IntrinsicType::OBJECT_CLASS: {
          // ClassDescriptor pointer
          elts.push_back(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(_context)));
          // GCInfo field - might be a forwarding pointer.
          elts.push_back(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(_context)));
          break;
        }

        case IntrinsicType::FLEXALLOC_CLASS: {
          elts.push_back(get(IntrinsicDefns::get()->objectClass->type(), {}));
          break;
        }

        default:
          assert(false && "Invalid class intrinsic");
      }
    } else if (td->extends().empty()) {
      // Object base class.
      elts.push_back(get(IntrinsicDefns::get()->objectClass->type(), typeArgs));
    } else {
      // TODO: Do we need to compose type args here?
      auto base = unwrapSpecialization(td->extends()[0], typeArgs);
      auto baseDefn = cast<TypeDefn>(base);
      auto baseType = baseDefn->type();
      elts.push_back(get(baseType, typeArgs));
      if (baseDefn->intrinsic() == IntrinsicType::FLEXALLOC_CLASS) {
        flexAllocElementType = getMemberType(typeArgs[0], {});
      }
    }

    // Data members
    for (auto member : td->members()) {
      if (member->kind == Defn::Kind::VAR_DEF && !member->isStatic()) {
        auto vd = static_cast<ValueDefn*>(member);
        if (!(vd->isConstant() && vd->isConstantInit())) {
          elts.push_back(getMemberType(vd->type(), typeArgs));
        }
      }
    }

    // FlexAlloc inserts a variable-length array at the end of the structure.
    if (flexAllocElementType) {
      elts.push_back(llvm::ArrayType::get(flexAllocElementType, 0));
    }

    return llvm::StructType::create(elts, linkageName);
  }

  CGUnionType* CGTypeBuilder::createUnion(const UnionType* ut) {
    auto it = _unions.find(ut);
    if (it != _unions.end()) {
      return it->second.get();
    }

    auto cgu = std::make_unique<CGUnionType>();
    for (auto m : ut->members) {
      if (m->kind == Type::Kind::VOID) {
        cgu->hasVoidType = true;
      } else if (m->kind == Type::Kind::CLASS) {
        cgu->hasRefType = true;
      } else if (m->kind == Type::Kind::INTERFACE) {
        cgu->hasInterfaceType = true;
      } else {
        cgu->valueTypes.push_back(m);
      }
    }

    uint64_t largestSize = 0;
    unsigned int largestAlign = 0;

    if (!cgu->valueTypes.empty()) {
      for (auto mt : cgu->valueTypes) {
        auto ty = getMemberType(mt, {});
        auto size = _dataLayout->getTypeStoreSize(ty);
        if (size > largestSize) {
          largestSize = size;
          cgu->valueType = ty;
        }
        largestAlign = std::max(largestAlign, _dataLayout->getABITypeAlignment(ty));
      }
      if (cgu->hasRefType) {
        auto objectPtr = getMemberType(IntrinsicDefns::get()->objectClass->type(), {});
        auto size = _dataLayout->getTypeStoreSize(objectPtr);
        if (size > largestSize) {
          largestSize = size;
          cgu->valueType = objectPtr;
        }
        largestAlign = std::max(largestAlign, _dataLayout->getABITypeAlignment(objectPtr));
      }
      if (cgu->hasInterfaceType) {
        assert(false && "Implement");
      }

      uint64_t tagSize;
      if (cgu->valueTypes.size() < 0x100 - 3) {
        tagSize = 1;
      } else {
        tagSize = 2;
        if (cgu->valueTypes.size() >= 0x10000 - 3) {
          diag.error() << "Union type with over 64K members!";
        }
      }

      // First field is the tag
      llvm::SmallVector<llvm::Type*, 16> elts;
      llvm::SmallVector<llvm::Type*, 16> ssaElts;

      cgu->tagType = llvm::IntegerType::get(_context, tagSize * 8);
      elts.push_back(cgu->tagType);
      ssaElts.push_back(cgu->tagType);

      // Optional padding
      cgu->valueStructIndex = 1;
      if (largestAlign > tagSize) {
        cgu->valueStructIndex = 2;
        elts.push_back(
            llvm::ArrayType::get(llvm::IntegerType::get(_context, 8), largestAlign - tagSize));
      }

      // Next comes an int whose alignment is size is equal to the largest alignment
      elts.push_back(cgu->valueType);
      ssaElts.push_back(cgu->valueType);

      // Packed struct
      cgu->type = llvm::StructType::create(elts, "union", true);

      // SSA value contains tag and pointer to data.
      cgu->ssaType = llvm::StructType::create(ssaElts, "unionSSA");
    } else if (cgu->hasInterfaceType) {
      assert(false && "Implement");
    } else if (cgu->hasRefType) {
      cgu->type = cgu->valueType = getMemberType(IntrinsicDefns::get()->objectClass->type(), {});
    } else {
      assert(false && "Empty union?");
    }

    auto result = cgu.get();
    _unions[ut] = std::move(cgu);
    return result;
  }

  llvm::Type* CGTypeBuilder::getObjectType() {
    if (!_objectType) {
      _objectType = get(intrinsic::IntrinsicDefns::get()->objectClass->type(), {});
    }
    return _objectType;
  }

  llvm::StructType* CGTypeBuilder::getClassDescType() {
    if (!_classDescType) {
      // Class descriptor fields:
      // - base class
      // - interface table
      // - method table
      auto cdType = llvm::StructType::create(_context, "ClassDescriptor");
      llvm::Type* descFieldTypes[3] = {
        cdType->getPointerTo(),
        llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo(),
        llvm::Type::getVoidTy(_context)->getPointerTo()->getPointerTo(),
      };
      cdType->setBody(descFieldTypes);
      _classDescType = cdType;
    }
    return _classDescType;
  }
}
