#include "tempest/sema/graph/typeorder.h"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/sema/graph/defn.h"

namespace tempest::sema::graph {
  bool TypeOrder::operator()(const Type* lhs, const Type* rhs) const {
    // Equality
    if (lhs == rhs) {
      return false;
    }

    // Different kinds
    if (lhs->kind != rhs->kind) {
      return lhs->kind < rhs->kind;
    }

    switch (lhs->kind) {
      case Type::Kind::VOID:
      case Type::Kind::BOOLEAN:
        return false;

      case Type::Kind::INTEGER: {
        auto intLhs = static_cast<const IntegerType*>(lhs);
        auto intRhs = static_cast<const IntegerType*>(rhs);
        if (intLhs->bits() != intRhs->bits()) {
          return intLhs->bits() < intRhs->bits();
        }
        if (intLhs->isUnsigned() != intRhs->isUnsigned()) {
          return intLhs->isUnsigned() < intRhs->bits();
        }
        if (intLhs->isPositive() != intRhs->isPositive()) {
          return intLhs->isPositive() < intRhs->isPositive();
        }
        return false;
      }

      case Type::Kind::FLOAT: {
        auto floatLhs = static_cast<const FloatType*>(lhs);
        auto floatRhs = static_cast<const FloatType*>(rhs);
        if (floatLhs->bits() != floatRhs->bits()) {
          return floatLhs->bits() < floatRhs->bits();
        }
        return false;
      }

      case Type::Kind::TUPLE: {
        auto tupleLhs = static_cast<const TupleType*>(lhs);
        auto tupleRhs = static_cast<const TupleType*>(rhs);
        auto largest = std::max(tupleLhs->members.size(), tupleRhs->members.size());
        for (size_t i = 0; i < largest; ++i) {
          if (i >= tupleLhs->members.size()) {
            return true;
          }
          if (i >= tupleRhs->members.size()) {
            return false;
          }

          auto mLhs = tupleLhs->members[i];
          auto mRhs = tupleRhs->members[i];
          if (this->operator()(mLhs, mRhs)) {
            return true;
          }
          if (this->operator()(mRhs, mLhs)) {
            return false;
          }
        }
        return false;
      }

      case Type::Kind::UNION: {
        auto unionLhs = static_cast<const UnionType*>(lhs);
        auto unionRhs = static_cast<const UnionType*>(rhs);
        auto largest = std::max(unionLhs->members.size(), unionRhs->members.size());
        for (size_t i = 0; i < largest; ++i) {
          if (i >= unionLhs->members.size()) {
            return true;
          }
          if (i >= unionRhs->members.size()) {
            return false;
          }

          auto mLhs = unionLhs->members[i];
          auto mRhs = unionRhs->members[i];
          if (this->operator()(mLhs, mRhs)) {
            return true;
          }
          if (this->operator()(mRhs, mLhs)) {
            return false;
          }
        }
        return false;
      }

      case Type::Kind::CLASS:
      case Type::Kind::STRUCT:
      case Type::Kind::INTERFACE:
      case Type::Kind::TRAIT:
      case Type::Kind::EXTENSION:
      case Type::Kind::ENUM: {
        auto udtLhs = static_cast<const UserDefinedType*>(lhs);
        auto udtRhs = static_cast<const UserDefinedType*>(rhs);
        return memberOrder(udtLhs->defn(), udtRhs->defn());
      }

      default:
        assert(false && "Unsupported type kind");
    }
  }

  bool TypeOrder::memberOrder(const Member* lhs, const Member* rhs) const {
    // Equality
    if (lhs == rhs) {
      return false;
    }

    if (lhs->definedIn() != rhs->definedIn()) {
      if (lhs->definedIn() == nullptr) {
        return true;
      }
      if (rhs->definedIn() == nullptr) {
        return false;
      }
      return memberOrder(lhs->definedIn(), rhs->definedIn());
    }

    // Different kinds
    if (lhs->kind != rhs->kind) {
      return lhs->kind < rhs->kind;
    }

    if (lhs->kind == Member::Kind::SPECIALIZED) {
      auto specLhs = static_cast<const SpecializedDefn*>(lhs);
      auto specRhs = static_cast<const SpecializedDefn*>(rhs);
      if (specLhs->base() != specRhs->base()) {
        return memberOrder(specLhs->base(), specRhs->base());
      }
      assert(specLhs->typeArgs().size() == specRhs->typeArgs().size());
      for (size_t i = 0; i < specLhs->typeArgs().size(); ++i) {
        auto mLhs = specLhs->typeArgs()[i];
        auto mRhs = specRhs->typeArgs()[i];
        if (this->operator()(mLhs, mRhs)) {
          return true;
        }
        if (this->operator()(mRhs, mLhs)) {
          return false;
        }
      }
      return false;
    }

    return lhs->name() < rhs->name();
  }
}
