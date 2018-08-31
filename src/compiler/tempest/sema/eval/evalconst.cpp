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
        result.hasSize = llvm::cast<IntegerType>(intLit->type)->bits() > 0;
        result.hasSign = !llvm::cast<IntegerType>(intLit->type)->isUnsigned();
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
        if (!evalConstExpr(op->arg, argResult)) {
          return false;
        }

        if (argResult.type == EvalResult::INT) {
          result.type = EvalResult::INT;
          result.hasSign = argResult.hasSign;
          result.hasSize = argResult.hasSize;

          switch (e->kind) {
            case Expr::Kind::NEGATE:
              if (!argResult.hasSign) {
                diag.error(op) << "Can't negate an unsigned number";
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
        if (!evalConstExpr(op->lhs, lhsResult) || !evalConstExpr(op->rhs, rhsResult)) {
          return false;
        }

        if (lhsResult.type == EvalResult::INT && rhsResult.type == EvalResult::INT) {
          result.type = EvalResult::INT;
          result.hasSign = lhsResult.hasSign || rhsResult.hasSign;
          result.hasSize = lhsResult.hasSize || rhsResult.hasSize;
          // Only sized types can be unsigned. So it's illegal for a type to be both unsized
          // and unsigned.
          assert(lhsResult.hasSign || lhsResult.hasSize);
          assert(rhsResult.hasSign || rhsResult.hasSize);

          // If one of the types is sized, and the other is not, then make the unsized one
          // the same as the sized one.
          if (lhsResult.hasSize) {
            if (rhsResult.hasSize) {
              // Both are sized, now let's see if the signs are the same.
              if (lhsResult.hasSign != rhsResult.hasSign) {
                diag.error(op) << "Signed / unsigned mismatch";
                return false;
              }
            } else {
              // Cannot convert negative unsized number to unsigned
              if (!lhsResult.hasSign && rhsResult.intResult.isNegative()) {
                diag.error(op) << "Cannot convert negative number to signed integer";
                return false;
              }
            }
          } else if (rhsResult.hasSize) {
            // Cannot convert negative unsized number to unsigned
            if (!rhsResult.hasSign && lhsResult.intResult.isNegative()) {
              diag.error(op) << "Cannot convert negative number to signed integer";
              return false;
            }
          }

          // See if we need to extend one or the other.
          uint32_t bitWidth = std::max(
              lhsResult.intResult.getBitWidth(),
              rhsResult.intResult.getBitWidth());
          if (lhsResult.intResult.getBitWidth() < bitWidth) {
            if (lhsResult.hasSign) {
              lhsResult.intResult = lhsResult.intResult.sext(bitWidth);
            } else {
              lhsResult.intResult = lhsResult.intResult.zext(bitWidth);
            }
          } else if (rhsResult.intResult.getBitWidth() < bitWidth) {
            if (rhsResult.hasSign) {
              rhsResult.intResult = rhsResult.intResult.sext(bitWidth);
            } else {
              rhsResult.intResult = rhsResult.intResult.zext(bitWidth);
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
              if (lhsResult.hasSign) {
                result.intResult = lhsResult.intResult.sdiv(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.udiv(rhsResult.intResult);
              }
              break;

            case Expr::Kind::REMAINDER:
              if (lhsResult.hasSign) {
                result.intResult = lhsResult.intResult.srem(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.urem(rhsResult.intResult);
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
              if (lhsResult.hasSign) {
                result.intResult = lhsResult.intResult.ashr(rhsResult.intResult);
              } else {
                result.intResult = lhsResult.intResult.lshr(rhsResult.intResult);
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
              if (lhsResult.hasSign) {
                result.boolResult = lhsResult.intResult.sle(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.ule(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::LT:
              if (lhsResult.hasSign) {
                result.boolResult = lhsResult.intResult.slt(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.ult(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::GE:
              if (lhsResult.hasSign) {
                result.boolResult = lhsResult.intResult.sge(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.uge(rhsResult.intResult);
              }
              result.type = EvalResult::BOOL;
              break;

            case Expr::Kind::GT:
              if (lhsResult.hasSign) {
                result.boolResult = lhsResult.intResult.sgt(rhsResult.intResult);
              } else {
                result.boolResult = lhsResult.intResult.ugt(rhsResult.intResult);
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
          diag.debug(e->location) << "Reference to variable '" << var->defn->name() <<
              "' is not a compile-time constant.";
          return false;
        }
      }

      default:
        diag.debug(e->location) << "Not a constant: " << Expr::KindName(e->kind);
        return false;
    }
  }
}
