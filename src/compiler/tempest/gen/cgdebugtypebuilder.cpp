#include "tempest/gen/cgdebugtypebuilder.h"
#include "tempest/gen/cgtypebuilder.h"
#include "tempest/gen/linkagename.h"
#include "tempest/intrinsic/defns.h"
#include "tempest/sema/graph/primitivetype.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Path.h>
#include <assert.h>

namespace tempest::gen {
  using llvm::DIType;
  using llvm::DIFile;
  using llvm::DINode;
  using llvm::DIScope;
  using llvm::DICompositeType;
  using llvm::DIDerivedType;
  using namespace tempest::sema::graph;
  using tempest::source::ProgramSource;
  using tempest::gen::getLinkageName;
  using tempest::intrinsic::IntrinsicDefns;

  // DIFlags:
  // Private = 1_u32
  // Protected = 2_u32
  // Public = 3_u32
  // Virtual = 32_u32
  // Artificial = 64_u32
  // StaticMember = 4096_u32
  // SingleInheritance = 65536_u32
  // MultipleInheritance = 131072_u32

  llvm::DIType* CGDebugTypeBuilder::get(const Type* ty) {
    switch (ty->kind) {
      case Type::Kind::VOID: {
        return _builder.createUnspecifiedType("void");
      }

      case Type::Kind::BOOLEAN: {
        return _builder.createBasicType("bool", 1, llvm::dwarf::DW_ATE_boolean);
      }

      case Type::Kind::INTEGER: {
        auto intTy = static_cast<const IntegerType*>(ty);
        return _builder.createBasicType(
            intTy->name(), intTy->bits(),
            intTy->isUnsigned() ? llvm::dwarf::DW_ATE_unsigned : llvm::dwarf::DW_ATE_signed);
      }

      case Type::Kind::FLOAT: {
        auto floatTy = static_cast<const FloatType*>(ty);
        return _builder.createBasicType(
            floatTy->name(), floatTy->bits(),
            llvm::dwarf::DW_ATE_float);
      }

      case Type::Kind::TUPLE: {
        auto tupleTy = static_cast<const TupleType*>(ty);
        (void)tupleTy;
        assert(false && "Implement DIType for tuples");
        // llvm::SmallVector<llvm::Type*, 16> elts;
        // for (auto member : tupleTy->members) {
        //   elts.push_back(getMemberType(member, typeArgs));
        // }
        // return llvm::StructType::create(elts);
      }

      case Type::Kind::CLASS: {
        auto cls = static_cast<const UserDefinedType*>(ty);
        auto td = cls->defn();
        auto it = _typeDefns.find(td);
        if (it != _typeDefns.end()) {
          return it->second;
        }
        auto irType = createClass(cls);
        _typeDefns[td] = irType;
        return irType;
      }

      // // Nominal types
      // STRUCT, INTERFACE, TRAIT, EXTENSION, ENUM,  // Composites
      // ALIAS,          // An alias for another type
      // TYPE_VAR,       // Reference to a template parameter

      // // Derived types
      // CONST,          // Const type modifier

      case Type::Kind::FUNCTION: {
        return createFunctionType(static_cast<const FunctionType*>(ty));
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

  llvm::DISubroutineType* CGDebugTypeBuilder::createFunctionType(const FunctionType* ft) {
    llvm::SmallVector<llvm::Metadata*, 8> paramTypes;
    // TODO: This should be param type or return type.
    paramTypes.push_back(getMemberType(ft->returnType));
    for (auto paramType : ft->paramTypes) {
      paramTypes.push_back(getMemberType(paramType));
    }
    return _builder.createSubroutineType(_builder.getOrCreateTypeArray(paramTypes));
  }

  llvm::DIType* CGDebugTypeBuilder::getMemberType(const Type* ty) {
    llvm::DIType* result = get(ty);
    if (ty->kind == Type::Kind::CLASS) {
      return _builder.createPointerType(result, 0);
    }
    return result;
  }

  DICompositeType* CGDebugTypeBuilder::createClass(const UserDefinedType* cls) {
    auto td = cls->defn();
    llvm::SmallVector<llvm::Metadata*, 16> elts;

    auto it = _typeDefns.find(td);
    if (it != _typeDefns.end()) {
      return llvm::cast<DICompositeType>(it->second);
    }

    // Qualified name
    std::string linkageName;
    linkageName.reserve(64);
    getLinkageName(linkageName, td);

    DIFile* diFile = getDIFile(td->location().source);
    DIScope* diScope = _diCompileUnit;

    auto irCls = llvm::cast<llvm::StructType>(_typeBuilder.get(cls));
    auto structLayout = _dataLayout->getStructLayout(irCls);

    // Base class
    if (td->intrinsic() != IntrinsicType::NONE) {
      switch (td->intrinsic()) {
        case IntrinsicType::OBJECT_CLASS: {
          // ClassDescriptor pointer
          elts.push_back(
            _builder.createMemberType(diScope, "__clsDesc", diFile, 0,
                _dataLayout->getPointerSizeInBits(),
                _dataLayout->getPointerPrefAlignment(),
                0,
                DINode::DIFlags::FlagZero,
                _builder.createPointerType(
                  _builder.createUnspecifiedType("__clsDesc"), 0)));

          // GCInfo field - might be a forwarding pointer.
          elts.push_back(
            _builder.createMemberType(diScope, "__gc_info", diFile, 0,
                _dataLayout->getPointerSizeInBits(),
                _dataLayout->getPointerPrefAlignment(),
                _dataLayout->getPointerSizeInBits(),
                DINode::DIFlags::FlagZero,
                _builder.createPointerType(
                  _builder.createUnspecifiedType("__gc_info"), 0)));
          break;
        }

        default:
          assert(false && "Invalid class intrinsic");
      }
    } else if (cls->extends().empty()) {
      // Object base class.
      elts.push_back(
        get(IntrinsicDefns::get()->objectClass->type())
      );
    } else {
      auto base = cls->extends()[0];
      elts.push_back(get(base));
    }
    // Data members
    int32_t memberIndex = 1; // Start from 1.
    for (auto member : td->members()) {
      if (member->kind == Defn::Kind::LET_DEF && !member->isStatic()) {
        auto vd = static_cast<ValueDefn*>(member);
        auto memberType = _typeBuilder.getMemberType(vd->type());
        elts.push_back(
          _builder.createMemberType(
            diScope, member->name(), diFile, member->location().startLine,
            _dataLayout->getTypeSizeInBits(memberType),
            _dataLayout->getPrefTypeAlignment(memberType),
            structLayout->getElementOffsetInBits(memberIndex),
            DINode::DIFlags::FlagZero, getMemberType(vd->type())));
      }
      memberIndex += 1;
    }
    DICompositeType* diCls = _builder.createClassType(
        diScope, td->name(), diFile, td->location().startLine,
        structLayout->getSizeInBits(),
        structLayout->getAlignment(), 0,
        DINode::DIFlags::FlagZero, nullptr, _builder.getOrCreateArray(elts));
    _typeDefns[td] = diCls;
    return diCls;
  }

  llvm::DIDerivedType* CGDebugTypeBuilder::createMember(const ValueDefn*) {
    return nullptr;
  }

  DIFile* CGDebugTypeBuilder::getDIFile(ProgramSource* src) {
    if (src) {
      return _builder.createFile(
        llvm::sys::path::filename(src->path()),
        llvm::sys::path::parent_path(src->path()));
    } else {
      return _diCompileUnit->getFile();
    }
  }
}
