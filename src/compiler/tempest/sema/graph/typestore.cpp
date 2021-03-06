#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/typestore.hpp"
#include "tempest/sema/graph/typeorder.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::graph {
  using tempest::error::diag;

  /** A hashable expression for singletons. */
  bool SingletonKey::equals(const SingletonKey& sk) const {
    if (sk._value->kind != _value->kind) {
      return false;
    }
    switch (_value->kind) {
      case Expr::Kind::INTEGER_LITERAL: {
        auto lhs = static_cast<const IntegerLiteral*>(_value);
        auto rhs = static_cast<const IntegerLiteral*>(sk._value);
        size_t maxWidth = std::max(lhs->intType()->bits(), rhs->intType()->bits());
        // Ugh, memory allocation in a key comparison.
        auto lhsVal = lhs->asAPInt().sext(maxWidth);
        auto rhsVal = rhs->asAPInt().sext(maxWidth);
        return lhsVal.eq(rhsVal);
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

  TypeStore::~TypeStore() {
    // Clear out all of the maps before the allocator goes away.
    _unionTypes.clear();
    _tupleTypes.clear();
    _functionTypes.clear();
    _modifiedTypes.clear();
    _singletonTypes.clear();
    _addressTypes.clear();
    _alloc.Reset();
  }

  UnionType* TypeStore::createUnionType(const TypeArray& members) {
    // Sort members by pointer. This makes the type key hash independent of order.
    llvm::SmallVector<const Type*, 8> sortedMembers(members.begin(), members.end());
    std::sort(sortedMembers.begin(), sortedMembers.end(), TypeOrder());

    // Return matching union instance if already exists.
    TypeKey key(sortedMembers);
    auto it = _unionTypes.find(key);
    if (it != _unionTypes.end()) {
      return it->second;
    }

    // Allocate type arrays of both the original and sorted orders.
    auto membersCopy = _alloc.copyOf(sortedMembers);
    auto ut = new (_alloc) UnionType(membersCopy);
    _unionTypes[TypeKey(membersCopy)] = ut;
    return ut;
  }

  TupleType* TypeStore::createTupleType(const TypeArray& members) {
    TypeKey key(members);
    auto it = _tupleTypes.find(key);
    if (it != _tupleTypes.end()) {
      return it->second;
    }

    auto keyCopy = _alloc.copyOf(members);
    auto tt = new (_alloc) TupleType(keyCopy);
    _tupleTypes[TypeKey(keyCopy)] = tt;
    return tt;
  }

  const ModifiedType* TypeStore::createModifiedType(const Type* base, uint32_t modifiers) {
    if (auto mt = dyn_cast<ModifiedType>(base)) {
      if (mt->modifiers == modifiers) {
        return mt;
      }
      base = mt->base;
    }

    auto key = std::pair<const Type*, uint32_t>(base, modifiers);
    auto it = _modifiedTypes.find(key);
    if (it != _modifiedTypes.end()) {
      return it->second;
    }

    auto ct = new (_alloc) ModifiedType(base, modifiers);
    _modifiedTypes[key] = ct;
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

  FunctionType* TypeStore::createFunctionType(
      const Type* returnType,
      const TypeArray& paramTypes,
      bool isVariadic,
      bool isMutableSelf) {
    assert(returnType->kind != Type::Kind::INVALID);
    llvm::SmallVector<const Type*, 8> signature;
    signature.reserve(paramTypes.size() + 1);
    signature.push_back(returnType);
    signature.insert(signature.end(), paramTypes.begin(), paramTypes.end());
    auto key = FunctionTypeKey(signature, isMutableSelf, isVariadic);
    auto it = _functionTypes.find(key);
    if (it != _functionTypes.end()) {
      return it->second;
    }
    auto paramTypesCopy = _alloc.copyOf(paramTypes);
    auto signatureCopy = _alloc.copyOf(signature);
    auto ft = new (_alloc) FunctionType(
        returnType, paramTypesCopy, isMutableSelf, isVariadic);
    _functionTypes[FunctionTypeKey(signatureCopy, isMutableSelf, isVariadic)] = ft;
    return ft;
  }

  IntegerType* TypeStore::createIntegerType(llvm::APInt& intVal, bool isUnsigned) {
    int32_t bits = intVal.getMinSignedBits();
    IntKey key({ bits, intVal.isNegative(), isUnsigned });
    auto it = _intTypes.find(key);
    if (it != _intTypes.end()) {
      return it->second;
    }

    auto nt = new (_alloc) IntegerType("integer", bits, isUnsigned, intVal.isNegative());
    _intTypes[key] = nt;
    return nt;
  }
}
