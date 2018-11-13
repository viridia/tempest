#include "tempest/sema/eval/evalconst.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/names/createnameref.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/pass/transform/loweroperators.hpp"

namespace tempest::sema::pass::transform {
  using namespace tempest::sema::graph;

  Expr* LowerOperatorsTransform::visit(Expr* e) {
    if (e == nullptr) {
      return e;
    }

    switch (e->kind) {
      case Expr::Kind::ADD:
      case Expr::Kind::SUBTRACT:
      case Expr::Kind::MULTIPLY:
      case Expr::Kind::DIVIDE:
      case Expr::Kind::REMAINDER:
      case Expr::Kind::LSHIFT:
      case Expr::Kind::RSHIFT:
      case Expr::Kind::BIT_OR:
      case Expr::Kind::BIT_AND:
      case Expr::Kind::BIT_XOR:
      case Expr::Kind::NE:
      case Expr::Kind::EQ:
      case Expr::Kind::LE:
      case Expr::Kind::LT:
      case Expr::Kind::GE:
      case Expr::Kind::GT:
        return visitInfixOperator(static_cast<BinaryOp*>(e));

      case Expr::Kind::NEGATE:
      case Expr::Kind::COMPLEMENT:
        return visitUnaryOperator(static_cast<UnaryOp*>(e));

      default:
        return ExprVisitor::visit(e);
    }
  }

  Expr* LowerOperatorsTransform::visitInfixOperator(BinaryOp* op) {
    if (auto evalResult = evalConstOperator(op)) {
      return evalResult;
    }

    StringRef funcName;
    llvm::ArrayRef<Expr*> args(op->args);
    bool inverted = false;
    switch (op->kind) {
      case Expr::Kind::ADD:
        funcName = "infixAdd";
        break;
      case Expr::Kind::SUBTRACT:
        funcName = "infixSubtract";
        break;
      case Expr::Kind::MULTIPLY:
        funcName = "infixMultiply";
        break;
      case Expr::Kind::DIVIDE:
        funcName = "infixDivide";
        break;
      case Expr::Kind::REMAINDER:
        funcName = "infixRemainder";
        break;
      case Expr::Kind::LSHIFT:
        funcName = "infixLeftShift";
        break;
      case Expr::Kind::RSHIFT:
        funcName = "infixRightShift";
        break;
      case Expr::Kind::BIT_OR:
        funcName = "infixBitOr";
        break;
      case Expr::Kind::BIT_AND:
        funcName = "infixBitAnd";
        break;
      case Expr::Kind::BIT_XOR:
        funcName = "infixBitXOr";
        break;
      case Expr::Kind::EQ:
        funcName = "isEqualTo";
        break;
      case Expr::Kind::NE:
        funcName = "isEqualTo";
        inverted = true;
        break;
      case Expr::Kind::LT:
        funcName = "isLessThan";
        break;
      case Expr::Kind::LE:
        funcName = "isLessThanOrEqual";
        break;
      case Expr::Kind::GT:
        funcName = "isLessThan";
        // Swap arg order
        args = _alloc.copyOf(llvm::ArrayRef<Expr*>({ args[1], args[0] }));
        break;
      case Expr::Kind::GE:
        funcName = "isLessThanOrEqual";
        // Swap arg order
        args = _alloc.copyOf(llvm::ArrayRef<Expr*>({ args[1], args[0] }));
        break;
      default:
        assert(false && "Invalid binary op");
    }

    auto fnRef = resolveOperatorName(op->location, funcName);
    auto result = new (_alloc) ApplyFnOp(Expr::Kind::CALL, op->location, fnRef, args);
    if (inverted) {
      return new (_alloc) UnaryOp(Expr::Kind::NOT, result);
    }
    return result;
  }

  Expr* LowerOperatorsTransform::visitUnaryOperator(UnaryOp* op) {
    if (auto evalResult = evalConstOperator(op)) {
      return evalResult;
    }

    StringRef funcName;
    switch (op->kind) {
      case Expr::Kind::NEGATE:
        funcName = "unaryNegation";
        break;
      case Expr::Kind::COMPLEMENT:
        funcName = "unaryComplement";
        break;
      default:
        assert(false && "Invalid unary op");
    }

    auto fnRef = resolveOperatorName(op->location, funcName);
    return new (_alloc) ApplyFnOp(Expr::Kind::CALL, op->location, fnRef, op->arg);
  }

  Expr* LowerOperatorsTransform::evalConstOperator(Expr* op) {
    eval::EvalResult result;
    result.failSilentIfNonConst = true;
    if (eval::evalConstExpr(op, result)) {
      return evalResultToExpr(op->location, result);
    } else if (result.error) {
      return &Expr::ERROR;
    }

    return nullptr;
  }

  Expr* LowerOperatorsTransform::evalResultToExpr(
      const source::Location& location, eval::EvalResult& result) {
    switch (result.type) {
      case eval::EvalResult::INT: {
        auto intType = _cu.types().createIntegerType(result.intResult, result.isUnsigned);
        return new (_alloc) IntegerLiteral(
          location,
          _alloc.copyOf(result.intResult),
          intType);
      }
      case eval::EvalResult::FLOAT: {
        FloatType* ftype = &FloatType::F64;
        if (result.size == eval::EvalResult::F32) {
          ftype = &FloatType::F32;
        }
        return new (_alloc) FloatLiteral(
          location,
          result.floatResult,
          false,
          ftype);
      }
      case eval::EvalResult::BOOL:
        assert(false && "Implement");
        break;
      case eval::EvalResult::STRING:
        assert(false && "Implement");
        break;
    }
  }

  Expr* LowerOperatorsTransform::resolveOperatorName(const Location& loc, const StringRef& name) {
    MemberLookupResult result;
    _scope->lookup(name, result);
    if (result.size() > 0) {
      auto nr = createNameRef(_alloc, loc, result,
          cast_or_null<Defn>(_scope->subject()),
          nullptr, false, true);
      if (auto mref = dyn_cast<MemberListExpr>(nr)) {
        mref->isOperator = true;
      }
      return nr;
    } else {
      auto mref = new (_alloc) MemberListExpr(
          Expr::Kind::FUNCTION_REF_OVERLOAD, loc, name,
          llvm::ArrayRef<MemberAndStem>());
      mref->useADL = true;
      mref->isOperator = true;
      return mref;
    }
  }
}
