#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/transform/transform.hpp"

namespace tempest::sema::transform {
  using namespace tempest::sema::graph;
  using tempest::sema::infer::InferredType;
  using tempest::sema::infer::ContingentType;
  using tempest::error::diag;

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

  Expr* NonMutatingExprTransform::transform(Expr* expr) {
    if (expr == nullptr) {
      return nullptr;
    }

    switch (expr->kind) {
      case Expr::Kind::VOID:
      case Expr::Kind::BOOLEAN_LITERAL:
      case Expr::Kind::INTEGER_LITERAL:
      case Expr::Kind::FLOAT_LITERAL:
        return expr;

      case Expr::Kind::CALL: {
        auto callExpr = static_cast<ApplyFnOp*>(expr);
        auto type = transformType(callExpr->type);
        auto function = transform(callExpr->function);
        llvm::SmallVector<Expr*, 8> args;
        if (transformArray(callExpr->args, args) ||
            function != callExpr->function ||
            type != callExpr->type) {
          return new (_alloc) ApplyFnOp(
              callExpr->kind, callExpr->location, function, _alloc.copyOf(args), type);
        }
        return callExpr;
      }

      case Expr::Kind::FUNCTION_REF:
      case Expr::Kind::TYPE_REF:
      case Expr::Kind::VAR_REF: {
        auto dref = static_cast<DefnRef*>(expr);
        auto defn = transformMember(dref->defn);
        auto type = transformType(dref->type);
        auto stem = transform(dref->stem);
        if (defn != dref->defn || type != dref->type || stem != dref->stem) {
          return new (_alloc) DefnRef(dref->kind, dref->location, defn, stem, type);
        }
        return dref;
      }

      case Expr::Kind::FUNCTION_REF_OVERLOAD:
      case Expr::Kind::TYPE_REF_OVERLOAD: {
        assert(false && "Implement");
        // auto mref = static_cast<MemberListExpr*>(expr);
        // mref->stem = transform(mref->stem);
        // return mref;
      }

      case Expr::Kind::BLOCK: {
        auto block = static_cast<BlockStmt*>(expr);
        auto result = transform(block->result);
        auto type = transformType(block->type);
        llvm::SmallVector<Expr*, 8> stmts;
        if (transformArray(block->stmts, stmts) || result != block->result || type != block->type) {
          return new (_alloc) BlockStmt(block->location, _alloc.copyOf(stmts), result, type);
        }
        return block;
      }

      case Expr::Kind::LOCAL_VAR: {
        auto st = static_cast<LocalVarStmt*>(expr);
        auto type = transformType(st->defn->type());
        auto init = transform(st->defn->init());
        if (type != st->defn->type() || init != st->defn->init()) {
          auto var = new (_alloc) ValueDefn(
              st->defn->kind,
              st->defn->location(),
              st->defn->name(),
              st->defn->definedIn());
          var->setInit(init);
          var->setType(type);
          return new (_alloc) LocalVarStmt(st->location, var);
        }
        return st;
      }

      case Expr::Kind::IF: {
        auto stmt = static_cast<IfStmt*>(expr);
        auto test = transform(stmt->test);
        auto thenBlock = transform(stmt->thenBlock);
        auto elseBlock = transform(stmt->elseBlock);
        auto type = transformType(stmt->type);
        if (test != stmt->test ||
            thenBlock != stmt->thenBlock ||
            elseBlock != stmt->elseBlock ||
            type != stmt->type) {
          return new (_alloc) IfStmt(stmt->location, test, thenBlock, elseBlock, type);
        }
        return stmt;
      }

      case Expr::Kind::WHILE: {
        auto stmt = static_cast<WhileStmt*>(expr);
        auto test = transform(stmt->test);
        auto body = transform(stmt->body);
        auto type = transformType(stmt->type);
        if (test != stmt->test || body != stmt->body || type != stmt->type) {
          return new (_alloc) WhileStmt(stmt->location, test, body, type);
        }
        return stmt;
      }

      case Expr::Kind::RETURN:
      case Expr::Kind::THROW: {
        auto ret = static_cast<UnaryOp*>(expr);
        auto arg = transform(ret->arg);
        auto type = transformType(ret->type);
        if (arg != ret->arg || type != ret->type) {
          return new (_alloc) UnaryOp(ret->kind, arg, type);
        }
        return ret;
      }

  // def visitThrow(self, expr, cs):
  //   '''@type expr: spark.graph.graph.Throw
  //      @type cs: constraintsolver.ConstraintSolver'''
  //   if expr.hasArg():
  //     self.assignTypes(expr.getArg(), self.typeStore.getEssentialTypes()['throwable'].getType())
  //   return self.NO_RETURN

      case Expr::Kind::ADD:
      case Expr::Kind::SUBTRACT:
      case Expr::Kind::MULTIPLY:
      case Expr::Kind::DIVIDE:
      case Expr::Kind::REMAINDER: {
        auto op = static_cast<BinaryOp*>(expr);
        auto a0 = transform(op->args[0]);
        auto a1 = transform(op->args[1]);
        auto type = transformType(op->type);
        if (a0 != op->args[0] || a1 != op->args[1] || type != op->type) {
          return new (_alloc) BinaryOp(op->kind, a0, a1, type);
        }
        return op;
      }

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(expr->kind);
        assert(false && "Invalid expression kind");
    }
  }

  bool NonMutatingExprTransform::transformArray(
      const ArrayRef<Expr*>& in, llvm::SmallVectorImpl<Expr*>& out) {
    bool changed = false;
    for (auto el : in) {
      auto newEl = transform(el);
      if (newEl != el) {
        changed = true;
      }
    }
    return changed;
  }
}
