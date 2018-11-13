#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/convert/casting.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;
  using namespace llvm;
  using tempest::error::diag;

  Expr* TypeCasts::cast(const Type* dst, Expr* src) {
    auto srcType = src->type;
    if (srcType == dst) {
      return src;
    }

    while (dst->kind == Type::Kind::ALIAS) {
      auto dstAlias = static_cast<const UserDefinedType*>(dst);
      dst = dstAlias->defn()->aliasTarget();
    }

    switch (dst->kind) {
      case Type::Kind::INTEGER: {
        auto dstInt = static_cast<const IntegerType*>(dst);
        if (srcType->kind == Type::Kind::INTEGER) {
          auto srcInt = static_cast<const IntegerType*>(srcType);
          if (srcInt->bits() == dstInt->bits()) {
            return src;
          } else if (srcInt->bits() < dstInt->bits()) {
            if (dstInt->isUnsigned()) {
              return makeCast(Expr::Kind::CAST_ZERO_EXTEND, src, dst);
            } else {
              return makeCast(Expr::Kind::CAST_SIGN_EXTEND, src, dst);
            }
          } else if (srcInt->bits() > dstInt->bits()) {
            return makeCast(Expr::Kind::CAST_INT_TRUNCATE, src, dst);
          }
        } else {
          assert(false && "Unsupported cast");
        }
      }

      case Type::Kind::FLOAT: {
        auto dstFlt = static_cast<const FloatType*>(dst);
        if (auto srcFlt = dyn_cast<FloatType>(srcType)) {
          if (srcFlt->bits() == dstFlt->bits()) {
            return src;
          } else if (srcFlt->bits() < dstFlt->bits()) {
            return makeCast(Expr::Kind::CAST_FP_EXTEND, src, dst);
          } else {
            return makeCast(Expr::Kind::CAST_FP_TRUNC, src, dst);
          }
        } else {
          assert(false && "Unsupported cast");
        }
      }

      case Type::Kind::UNION: {
        if (srcType->kind == Type::Kind::UNION) {
          assert(false && "Implement");
        } else {
          auto ut = static_cast<const UnionType*>(dst);
          // First cast src to the type of the union member.
          ConversionResult bestResult;
          const Type* bestDst = nullptr;
          for (auto mt : ut->members) {
            auto result = isAssignable(mt, src->type);
            if (result.rank > bestResult.rank) {
              bestResult = result;
              bestDst = mt;
            }
          }
          assert(bestDst);
          assert(bestResult.rank >= ConversionRank::INEXACT);
          src = cast(bestDst, src);
          return makeCast(Expr::Kind::CAST_CREATE_UNION, src, dst);
        }
      }

      case Type::Kind::SPECIALIZED: {
        if (isEqual(dst, src->type)) {
          return src;
        }
        // auto dstSp = static_cast<const SpecializedType*>(dst);
        // if (auto srcSp = dyn_cast<SpecializedType>(src->type)) {

        // }
        assert(false && "Implement");
      }

      default:
        diag.fatal(src->location) << "Invalid cast: " << src->type << " => " << dst;
        break;
    }

    return src;
  }

  Expr* TypeCasts::makeCast(Expr::Kind kind, Expr* arg, const Type* type) {
    return new (_alloc) UnaryOp(kind, arg->location, arg, type);
  }
}
