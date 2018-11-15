#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/transform/visitor.hpp"

namespace tempest::sema::transform {
  using namespace tempest::sema::graph;
  using tempest::error::diag;

  Expr* ExprVisitor::visit(Expr* expr) {
    if (expr == nullptr) {
      return nullptr;
    }

    switch (expr->kind) {
      case Expr::Kind::INVALID:
      case Expr::Kind::VOID:
      case Expr::Kind::BOOLEAN_LITERAL:
      case Expr::Kind::INTEGER_LITERAL:
      case Expr::Kind::FLOAT_LITERAL:
      case Expr::Kind::SELF:
      case Expr::Kind::SUPER:
      case Expr::Kind::ALLOC_OBJ:
        return expr;

      case Expr::Kind::CALL: {
        auto callExpr = static_cast<ApplyFnOp*>(expr);
        callExpr->function = visit(callExpr->function);
        visitArray(callExpr->args);
        return callExpr;
      }

      case Expr::Kind::FUNCTION_REF:
      case Expr::Kind::TYPE_REF:
      case Expr::Kind::VAR_REF: {
        auto dref = static_cast<DefnRef*>(expr);
        dref->stem = visit(dref->stem);
        return dref;
      }

      case Expr::Kind::FUNCTION_REF_OVERLOAD:
      case Expr::Kind::TYPE_REF_OVERLOAD: {
        auto mref = static_cast<MemberListExpr*>(expr);
        mref->stem = visit(mref->stem);
        return mref;
      }

      case Expr::Kind::BLOCK: {
        auto block = static_cast<BlockStmt*>(expr);
        visitArray(block->stmts);
        block->result = visit(block->result);
        return block;
      }

      case Expr::Kind::LOCAL_VAR: {
        auto st = static_cast<LocalVarStmt*>(expr);
        st->defn->setInit(visit(st->defn->init()));
        return st;
      }

      case Expr::Kind::IF: {
        auto stmt = static_cast<IfStmt*>(expr);
        stmt->test = visit(stmt->test);
        stmt->thenBlock = visit(stmt->thenBlock);
        stmt->elseBlock = visit(stmt->elseBlock);
        return stmt;
      }

      case Expr::Kind::WHILE: {
        auto stmt = static_cast<WhileStmt*>(expr);
        stmt->test = visit(stmt->test);
        stmt->body = visit(stmt->body);
        return stmt;
      }

      case Expr::Kind::RETURN: {
        auto ret = static_cast<UnaryOp*>(expr);
        ret->arg = visit(ret->arg);
        return ret;
      }

  // def visitThrow(self, expr, cs):
  //   '''@type expr: spark.graph.graph.Throw
  //      @type cs: constraintsolver.ConstraintSolver'''
  //   if expr.hasArg():
  //     self.assignTypes(expr.getArg(), self.typeStore.getEssentialTypes()['throwable'].getType())
  //   return self.NO_RETURN

      case Expr::Kind::ASSIGN:
      case Expr::Kind::ADD:
      case Expr::Kind::SUBTRACT:
      case Expr::Kind::MULTIPLY:
      case Expr::Kind::DIVIDE:
      case Expr::Kind::REMAINDER:
      case Expr::Kind::BIT_AND:
      case Expr::Kind::BIT_OR:
      case Expr::Kind::BIT_XOR:
      case Expr::Kind::EQ:
      case Expr::Kind::NE:
      case Expr::Kind::LT:
      case Expr::Kind::LE:
      case Expr::Kind::GE:
      case Expr::Kind::GT: {
        auto op = static_cast<BinaryOp*>(expr);
        op->args[0] = visit(op->args[0]);
        op->args[1] = visit(op->args[1]);
        return op;
      }

      case Expr::Kind::NEGATE:
      case Expr::Kind::COMPLEMENT:
      case Expr::Kind::CAST_SIGN_EXTEND:
      case Expr::Kind::CAST_ZERO_EXTEND:
      case Expr::Kind::CAST_INT_TRUNCATE:
      case Expr::Kind::CAST_FP_EXTEND:
      case Expr::Kind::CAST_FP_TRUNC:
      case Expr::Kind::UNSAFE: {
        auto op = static_cast<UnaryOp*>(expr);
        op->arg = visit(op->arg);
        return op;
      }

      case Expr::Kind::MEMBER_NAME_REF: {
        auto mref = static_cast<MemberNameRef*>(expr);
        mref->stem = visit(mref->stem);
        mref->refs = visit(mref->refs);
        return mref;
      }

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(expr->kind);
        assert(false && "Invalid expression kind");
    }
  }

  void ExprVisitor::visitArray(const ArrayRef<Expr*>& in) {
    for (size_t i = 0; i < in.size(); i += 1) {
      const_cast<Expr**>(in.data())[i] = visit(in[i]);
    }
  }
}
