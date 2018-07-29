#include "tempest/gen/cgtypebuilder.h"
#include "tempest/gen/linkagename.h"
#include "tempest/intrinsic/defns.h"
#include "tempest/sema/graph/primitivetype.h"
#include <llvm/IR/DerivedTypes.h>
#include <assert.h>

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::gen::getLinkageName;
  using tempest::intrinsic::IntrinsicDefns;

  llvm::Type* CGTypeBuilder::get(const Type* ty) {
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

      case Type::Kind::TUPLE: {
        auto tupleTy = static_cast<const TupleType*>(ty);
        llvm::SmallVector<llvm::Type*, 16> elts;
        for (auto member : tupleTy->members) {
          elts.push_back(getMemberType(member));
        }
        return llvm::StructType::create(elts);
      }

      case Type::Kind::CLASS: {
        auto cls = static_cast<const UserDefinedType*>(ty);
        auto td = cls->defn();
        auto it = _typeDefns.find(td);
        if (it != _typeDefns.end()) {
          return it->second;
        }
        if (td->kind == Defn::Kind::SPECIALIZED) {
          assert(false && "Implement");
        } else {
          auto irType = createClass(cls);
          _typeDefns[td] = irType;
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
        for (auto param : fty->paramTypes) {
          // TODO: getParamType or getInternalParamType
          paramTypes.push_back(getMemberType(param));
        }
        auto returnType = getMemberType(fty->returnType);
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

  llvm::Type* CGTypeBuilder::getMemberType(const Type* ty) {
    llvm::Type* result = get(ty);
    if (ty->kind == Type::Kind::CLASS) {
      return llvm::PointerType::get(result, 1); // GC address space
    }
    return result;
  }

  llvm::Type* CGTypeBuilder::createClass(const UserDefinedType* cls) {
    auto td = cls->defn();
    llvm::SmallVector<llvm::Type*, 16> elts;

    // Qualified name
    std::string linkageName;
    linkageName.reserve(64);
    getLinkageName(linkageName, td);

    // Base class
    if (td->intrinsic() != IntrinsicType::NONE) {
      switch (td->intrinsic()) {
        case IntrinsicType::OBJECT_CLASS: {
          // ClassDescriptor pointer
          elts.push_back(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(_context)));
          // GCInfo field - might be a forwarding pointer.
          elts.push_back(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(_context)));
          break;
        }

        default:
          assert(false && "Invalid class intrinsic");
      }
    } else if (cls->extends().empty()) {
      // Object base class.
      elts.push_back(get(IntrinsicDefns::get()->objectClass->type()));
    } else {
      auto base = cls->extends()[0];
      elts.push_back(get(base));
    }
    // Data members
    for (auto member : td->members()) {
      if (member->kind == Defn::Kind::LET_DEF && !member->isStatic()) {
        auto vd = static_cast<ValueDefn*>(member);
        elts.push_back(getMemberType(vd->type()));
      }
    }
    return llvm::StructType::create(elts, linkageName);
  }
}
