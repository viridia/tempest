#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/eval/evalconst.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "llvm/Support/Casting.h"

namespace tempest::sema::eval {
  using namespace tempest::sema::graph;
  using tempest::error::diag;

  bool evalConstExpr(const Expr* e, EvalResult& result) {
    if (e == nullptr) {
      return false;
    }

    switch (e->kind) {
      case Expr::Kind::INTEGER_LITERAL: {
        auto intLit = static_cast<const IntegerLiteral*>(e);
        result.intResult = intLit->value();
        result.type = EvalResult::INT;
        result.hasSize = !llvm::cast<IntegerType>(intLit->type)->isImplicitlySized();
        result.isUnsigned = llvm::cast<IntegerType>(intLit->type)->isUnsigned();
        return true;
      }

      case Expr::Kind::STRING_LITERAL: {
        auto strLit = static_cast<const StringLiteral*>(e);
        result.strResult = strLit->value();
        result.type = EvalResult::STRING;
        return true;
      }

      case Expr::Kind::NEGATE:
      case Expr::Kind::COMPLEMENT: {
        auto op = static_cast<const UnaryOp*>(e);
        EvalResult argResult;
        argResult.failSilentIfNonConst = result.failSilentIfNonConst;
        if (!evalConstExpr(op->arg, argResult)) {
          result.error = argResult.error;
          return false;
        }

        if (argResult.type == EvalResult::INT) {
          result.type = EvalResult::INT;
          result.isUnsigned = argResult.isUnsigned;
          result.hasSize = argResult.hasSize;

          switch (e->kind) {
            case Expr::Kind::NEGATE:
              if (argResult.isUnsigned) {
                diag.error(op) << "Can't negate an unsigned number";
                result.error = true;
              }
              result.intResult = -argResult.intResult;
              break;

            case Expr::Kind::COMPLEMENT:
              result.intResult = ~argResult.intResult;
              break;

            default:
              return false;
          }

          return true;
        } else {
          // float
          // string
          assert(false && "Implement other data types for eval constant");
        }

        return true;
      }

      case Expr::Kind::ADD:
      case Expr::Kind::SUBTRACT:
      case Expr::Kind::MULTIPLY:
      case Expr::Kind::DIVIDE:
      case Expr::Kind::REMAINDER:
      case Expr::Kind::BIT_OR:
      case Expr::Kind::BIT_AND:
      case Expr::Kind::BIT_XOR:
      case Expr::Kind::LSHIFT:
      case Expr::Kind::RSHIFT:
      case Expr::Kind::EQ:
      case Expr::Kind::NE:
      case Expr::Kind::LE:
      case Expr::Kind::LT:
      case Expr::Kind::GE:
      case Expr::Kind::GT: {
        auto op = static_cast<const BinaryOp*>(e);
        EvalResult lhsResult;
        EvalResult rhsResult;
        lhsResult.failSilentIfNonConst = rhsResult.failSilentIfNonConst
            = result.failSilentIfNonConst;
        if (!evalConstExpr(op->args[0], lhsResult) || !evalConstExpr(op->args[1], rhsResult)) {
          result.error = lhsResult.error || rhsResult.error;
          return false;
        }

        if (lhsResult.type == EvalResult::INT && rhsResult.type == EvalResult::INT) {
          result.type = EvalResult::INT;
          result.isUnsigned = lhsResult.isUnsigned || rhsResult.isUnsigned;
          result.hasSize = lhsResult.hasSize || rhsResult.hasSize;

          // If one of the types is sized, and the other is not, then make the unsized one
          // the same as the sized one.
          if (lhsResult.hasSize) {
            if (rhsResult.hasSize) {
              // Both are sized, now let's see if the signs are the same.
              if (lhsResult.isUnsigned != rhsResult.isUnsigned) {
                diag.error(op) << "Signed / unsigned mismatch";
                result.error = true;
                return false;
              }
            } else {
              // Cannot convert negative unsized number to unsigned
              if (lhsResult.isUnsigned && rhsResult.intResult.isNegative()) {
                diag.error(op) << "Cannot convert negative number to signed integer";
                result.error = true;
                return false;
              }
            }
          } else if (rhsResult.hasSize) {
            // Cannot convert negative unsized number to unsigned
            if (rhsResult.isUnsigned && lhsResult.intResult.isNegative()) {
              diag.error(op) << "Cannot convert negative number to signed integer";
              result.error = true;
              return false;
            }
          }

          // See if we need to extend one or the other.
          uint32_t bitWidth = std::max(
              lhsResult.intResult.getBitWidth(),
              rhsResult.intResult.getBitWidth());
          if (lhsResult.intResult.getBitWidth() < bitWidth) {
            if (lhsResult.isUnsigned) {
              lhsResult.intResult = lhsResult.intResult.zext(bitWidth);
            } else {
              lhsResult.intResult = lhsResult.intResult.sext(bitWidth);
            }
          } else if (rhsResult.intResult.getBitWidth() < bitWidth) {
            if (rhsResult.isUnsigned) {
              rhsResult.intResult = rhsResult.intResult.zext(bitWidth);
            } else {
              rhsResult.intResult = rhsResult.intResult.sext(bitWidth);
            }
          }

          switch (e->kind) {
            case Expr::Kind::ADD:
              result.intResult = lhsResult.intResult + rhsResult.intResult;
              break;

            case Expr::Kind::SUBTRACT:
              result.intResult = lhsResult.intResult - rhsResult.intResult;
              break;

            case Expr::Kind::MULTIPLY:
              result.intResult = lhsResult.intResult * rhsResult.intResult;
              break;

            case Expr::Kind::DIVIDE:
              if (lhsResult.isUnsigned) {
                result.intResult = lhsResult.intResult.udiv(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.sdiv(rhsResult.intResult);
              }
              break;

            case Expr::Kind::REMAINDER:
              if (lhsResult.isUnsigned) {
                result.intResult = lhsResult.intResult.urem(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.srem(rhsResult.intResult);
              }
              break;

            case Expr::Kind::BIT_OR:
              result.intResult = lhsResult.intResult | rhsResult.intResult;
              break;

            case Expr::Kind::BIT_AND:
              result.intResult = lhsResult.intResult & rhsResult.intResult;
              break;

            case Expr::Kind::BIT_XOR:
              result.intResult = lhsResult.intResult ^ rhsResult.intResult;
              break;

            case Expr::Kind::LSHIFT:
              result.intResult = lhsResult.intResult.shl(rhsResult.intResult);
              break;

            case Expr::Kind::RSHIFT:
              if (lhsResult.isUnsigned) {
                result.intResult = lhsResult.intResult.lshr(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.ashr(rhsResult.intResult);
              }
              break;

            case Expr::Kind::EQ:
              result.boolResult = lhsResult.intResult == rhsResult.intResult;
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::NE:
              result.boolResult = lhsResult.intResult != rhsResult.intResult;
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::LE:
              if (lhsResult.isUnsigned) {
                result.boolResult = lhsResult.intResult.ule(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.sle(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::LT:
              if (lhsResult.isUnsigned) {
                result.boolResult = lhsResult.intResult.ult(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.slt(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::GE:
              if (lhsResult.isUnsigned) {
                result.boolResult = lhsResult.intResult.uge(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.sge(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::GT:
              if (lhsResult.isUnsigned) {
                result.boolResult = lhsResult.intResult.ugt(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.sgt(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            default:
              return false;
          }

          return true;
        } else {
          // float + float
          // string + string
          assert(false && "Implement other data types for eval constant");
        }

        return true;
      }

      case Expr::Kind::VAR_REF: {
        auto var = static_cast<const DefnRef*>(e);
        if (var->defn->kind == Member::Kind::ENUM_VAL) {
          auto varDef = static_cast<const ValueDefn*>(var->defn);
          if (varDef->init()) {
            // TODO: Ensure there are no cycles.
            return evalConstExpr(varDef->init(), result);
          } else {
            diag.debug(e->location) << "Enumeration value '" << var->defn->name() <<
                "' has not yet been assigned a value.";

          }
        } else {
          if (!result.failSilentIfNonConst) {
            diag.debug(e->location) << "Reference to variable '" << var->defn->name() <<
                "' is not a compile-time constant.";
          }
          return false;
        }
      }

      default:
        diag.debug(e->location) << "Not a constant: " << Expr::KindName(e->kind);
        return false;
    }
  }
}
