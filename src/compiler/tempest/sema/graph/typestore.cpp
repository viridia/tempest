#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/typestore.hpp"
#include "tempest/sema/graph/typeorder.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::graph {

  /** A hashable expression for singletons. */
  bool SingletonKey::equals(const SingletonKey& sk) const {
    if (sk._value->kind != _value->kind) {
      return false;
    }
    switch (_value->kind) {
      case Expr::Kind::INTEGER_LITERAL: {
        auto lhs = static_cast<const IntegerLiteral*>(_value);
        auto rhs = static_cast<const IntegerLiteral*>(sk._value);
        size_t maxWidth = std::max(lhs->value().getBitWidth(), rhs->value().getBitWidth());
        llvm::APInt lhsValue(lhs->value().sext(maxWidth));
        llvm::APInt rhsValue(rhs->value().sext(maxWidth));
        return lhsValue.eq(rhsValue);
      }

      case Expr::Kind::STRING_LITERAL: {
        auto lhs = static_cast<const StringLiteral*>(_value);
        auto rhs = static_cast<const StringLiteral*>(sk._value);
        return lhs->value() == rhs->value();
      }

      case Expr::Kind::BOOLEAN_LITERAL: {
        auto lhs = static_cast<const BooleanLiteral*>(_value);
        auto rhs = static_cast<const BooleanLiteral*>(sk._value);
        return lhs->value() == rhs->value();
      }

      default:
        assert(false && "Invalid singleton type");
    }
  }

  std::size_t SingletonKeyHash::operator()(const SingletonKey& sk) const {
    std::size_t hash = std::hash<size_t>()((size_t) sk.value()->kind);
    switch (sk.value()->kind) {
      case Expr::Kind::INTEGER_LITERAL: {
        auto value = static_cast<const IntegerLiteral*>(sk.value());
        hash_combine(hash, (size_t) llvm::hash_value(value->value()));
        break;
      }

      case Expr::Kind::STRING_LITERAL: {
        auto value = static_cast<const StringLiteral*>(sk.value());
        hash_combine(hash, (size_t) llvm::hash_value(value->value()));
        break;
      }

      case Expr::Kind::BOOLEAN_LITERAL: {
        auto value = static_cast<const BooleanLiteral*>(sk.value());
        hash_combine(hash, std::hash<bool>()(value->value()));
        break;
      }

      default:
        assert(false && "Invalid singleton type");
    }
    return hash;
  }

  UnionType* TypeStore::createUnionType(const TypeArray& members) {
    // Sort members by pointer. This makes the type key hash independent of order.
    llvm::SmallVector<Type*, 4> sortedMembers(members.begin(), members.end());
    std::sort(sortedMembers.begin(), sortedMembers.end(), TypeOrder());

    // Return matching union instance if already exists.
    TypeKey key(sortedMembers);
    auto it = _unionTypes.find(key);
    if (it != _unionTypes.end()) {
      return it->second;
    }

    // Allocate type arrays of both the original and sorted orders.
    auto membersCopy = new (_alloc) TypeArray(sortedMembers.begin(), sortedMembers.end());
    auto ut = new (_alloc) UnionType(*membersCopy);
    _unionTypes[TypeKey(ut->members)] = ut;
    return ut;
  }

  TupleType* TypeStore::createTupleType(const TypeArray& members) {
    TypeKey key(members);
    auto it = _tupleTypes.find(key);
    if (it != _tupleTypes.end()) {
      return it->second;
    }

    auto keyCopy = new (_alloc) TypeArray(key.begin(), key.end());
    auto tt = new (_alloc) TupleType(*keyCopy);
    _tupleTypes[TypeKey(tt->members)] = tt;
    return tt;
  }

  ConstType* TypeStore::createConstType(Type* base, bool provisional) {
    auto key = std::pair<Type*, bool>(base, provisional);
    auto it = _constTypes.find(key);
    if (it != _constTypes.end()) {
      return it->second;
    }

    auto ct = new (_alloc) ConstType(base, provisional);
    _constTypes[key] = ct;
    return ct;
  }

  SingletonType* TypeStore::createSingletonType(const Expr* expr) {
    SingletonKey key(expr);
    auto it = _singletonTypes.find(key);
    if (it != _singletonTypes.end()) {
      return it->second;
    }

    auto ct = new (_alloc) SingletonType(expr);
    _singletonTypes[key] = ct;
    return ct;
  }

  FunctionType* TypeStore::createFunctionType(Type* returnType, const TypeArray& paramTypes) {
    std::vector<Type*> signature;
    signature.reserve(paramTypes.size() + 1);
    signature.push_back(returnType);
    signature.insert(signature.end(), paramTypes.begin(), paramTypes.end());
    TypeKey key = TypeKey(signature);
    auto it = _functionTypes.find(key);
    if (it != _functionTypes.end()) {
      return it->second;
    }
    auto paramTypesCopy = new (_alloc) TypeArray(paramTypes);
    auto signatureCopy = new (_alloc) TypeArray(signature);
    auto ft = new (_alloc) FunctionType(returnType, *paramTypesCopy, false);
    _functionTypes[TypeKey(*signatureCopy)] = ft;
    return ft;
  }

  // FunctionType* TypeStore::createFunctionType(
  //     Type* returnType,
  //     const llvm::ArrayRef<ParameterDefn*>& params) {
  //   std::vector<Type*> paramTypes;
  //   paramTypes.reserve(params.size());
  //   for (auto param : params) {
  //     paramTypes.push_back(param->type());
  //   }
  //   return createFunctionType(returnType, paramTypes);
  // }

  /** Specialize a generic definition. */
  SpecializedDefn* TypeStore::specialize(GenericDefn* base, const TypeArray& typeArgs) {
    SpecializationKey key(base, typeArgs);
    auto it = _specs.find(key);
    if (it != _specs.end()) {
      return it->second;
    }

    auto spec = new (_alloc) SpecializedDefn(base, typeArgs, base->allTypeParams());
    if (auto typeDefn = llvm::dyn_cast<TypeDefn>(base)) {
      spec->setType(new (_alloc) SpecializedType(spec));
    }
    _specs[key] = spec;
    return spec;
  }
}
