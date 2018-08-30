#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/transform.hpp"

namespace tempest::sema::graph {
  using tempest::sema::infer::InferredType;
  using tempest::sema::infer::ContingentType;

  const Type* TypeTransform::transform(const Type* ty) {
    if (ty == nullptr) {
      return nullptr;
    }

    switch (ty->kind) {
      case Type::Kind::INVALID:
      case Type::Kind::NEVER:
      case Type::Kind::IGNORED:
      case Type::Kind::VOID:
      case Type::Kind::BOOLEAN:
      case Type::Kind::INTEGER:
      case Type::Kind::FLOAT:
      case Type::Kind::SINGLETON:
      case Type::Kind::CLASS:
      case Type::Kind::STRUCT:
      case Type::Kind::INTERFACE:
      case Type::Kind::TRAIT:
      case Type::Kind::EXTENSION:
      case Type::Kind::ENUM:
      case Type::Kind::ALIAS:
        return ty;

    //   case Type::Kind::ALIAS:
    //     assert(false && "Implement");
    //     break;

      case Type::Kind::MODIFIED: {
        auto ct = static_cast<const ModifiedType*>(ty);
        auto base = transform(ct->base);
        if (base != ct->base) {
          return new (_alloc) ModifiedType(base, ct->modifiers);
        }
        return ct;
      }

      case Type::Kind::UNION: {
        auto ut = static_cast<const UnionType*>(ty);
        llvm::SmallVector<const Type*, 8> members;
        if (transformArray(members, ut->members)) {
          return new (_alloc) UnionType(_alloc.copyOf(members));
        }
        return ut;
      }

      case Type::Kind::TUPLE: {
        auto tt = static_cast<const TupleType*>(ty);
        llvm::SmallVector<const Type*, 8> members;
        if (transformArray(members, tt->members)) {
          return new (_alloc) TupleType(_alloc.copyOf(members));
        }
        return tt;
      }

      case Type::Kind::SPECIALIZED: {
        auto st = static_cast<const SpecializedType*>(ty);
        auto sd = st->spec;
        llvm::SmallVector<const Type*, 8> typeArgs;
        if (transformArray(typeArgs, sd->typeArgs())) {
          auto nsd = new (_alloc) SpecializedDefn(
              sd->generic(), _alloc.copyOf(typeArgs), sd->typeParams());
          return new (_alloc) SpecializedType(nsd);
        }
        return st;
      }

      case Type::Kind::FUNCTION: {
        auto ft = static_cast<const FunctionType*>(ty);
        llvm::SmallVector<const Type*, 8> paramTypes;
        auto returnType = transform(ft->returnType);
        if (transformArray(paramTypes, ft->paramTypes) || returnType != ft->returnType) {
          return new (_alloc) FunctionType(
            returnType, _alloc.copyOf(paramTypes), ft->constSelf, ft->isVariadic);
        }
        return ft;
      }

      case Type::Kind::TYPE_VAR: {
        return transformTypeVar(static_cast<const TypeVar*>(ty));
      }

      case Type::Kind::CONTINGENT: {
        return transformContingentType(static_cast<const ContingentType*>(ty));
      }

      case Type::Kind::INFERRED:
        return transformInferredType(static_cast<const infer::InferredType*>(ty));

      default:
        assert(false && "TypeTransform - unsupported type kind");
    }
  }

  const Type* TypeTransform::transformContingentType(const infer::ContingentType* in) {
    llvm::SmallVector<ContingentType::Entry, 8> entries;
    bool changed = false;
    for (auto entry : in->entries) {
      ContingentType::Entry newEntry(entry);
      newEntry.type = transform(entry.type);
      if (newEntry.type != entry.type) {
        changed = true;
      }
    }

    if (!changed) {
      return in;
    }

    return new (_alloc) ContingentType(_alloc.copyOf(entries));
  }

  bool TypeTransform::transformArray(
      llvm::SmallVectorImpl<const Type*>& result,
      const TypeArray& in) {
    bool changed = false;
    for (auto ty : in) {
      auto tty = transform(ty);
      result.push_back(tty);
      if (tty != ty) {
        changed = true;
      }
    }
    return changed;
  }
}
