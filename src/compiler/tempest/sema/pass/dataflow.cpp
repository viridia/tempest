#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/pass/dataflow.hpp"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;

  void DataFlowPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void DataFlowPass::process(Module* mod) {
    // diag.info() << "Resolving imports: " << mod->name();
    _alloc = &mod->semaAlloc();
    visitList(mod->members());
  }

  void DataFlowPass::visitList(DefnArray members) {
    for (auto defn : members) {
      switch (defn->kind) {
        case Member::Kind::TYPE: {
          visitTypeDefn(static_cast<TypeDefn*>(defn));
          break;
        }

        case Member::Kind::FUNCTION: {
          visitFunctionDefn(static_cast<FunctionDefn*>(defn));
          break;
        }

        case Member::Kind::CONST_DEF:
        case Member::Kind::LET_DEF: {
          visitValueDefn(static_cast<ValueDefn*>(defn));
          break;
        }

        default:
          assert(false && "Shouldn't get here, bad member type.");
          break;
      }
    }
  }

  void DataFlowPass::visitTypeDefn(TypeDefn* td) {
    if (td->type()->kind == Type::Kind::ENUM) {
      visitEnumDefn(td);
    } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(td);
    }
  }

  void DataFlowPass::visitCompositeDefn(TypeDefn* td) {
    visitList(td->members());
  }

  void DataFlowPass::visitEnumDefn(TypeDefn* td) {
    // Do nothing
  }

  void DataFlowPass::visitFunctionDefn(FunctionDefn* fd) {
    if (fd->body()) {
      FlowState flow;
      size_t numLocalVars = fd->localDefns().size();
      flow.localVarsSet.resize(numLocalVars);
      flow.localVarsRead.resize(numLocalVars);
      if (auto clsDef = dyn_cast<TypeDefn>(fd->definedIn())) {
        flow.memberVarsSet.resize(clsDef->numInstanceVars());
      }

      auto prevCurrentMethod = _currentMethod;
      _currentMethod = fd;
      visitExpr(fd->body(), flow);
      _currentMethod = prevCurrentMethod;

      for (auto ld : fd->localDefns()) {
        if (auto vd = dyn_cast<ValueDefn>(ld)) {
          if (!flow.localVarsRead[vd->fieldIndex()]) {
            if (!flow.localVarsSet[vd->fieldIndex()]) {
              diag.error(ld) << "Unused local variable '" << ld->name() << "'.";
            } else {
              diag.error(ld) << "Local variable '" << ld->name() << "' is never read.";
            }
          }
        }
      }
    }
  }

  void DataFlowPass::visitValueDefn(ValueDefn* vd) {
    // Nothing
  }

  void DataFlowPass::visitExpr(Expr* e, FlowState& flow) {
    if (!e) {
      return;
    }

    switch (e->kind) {
      case Expr::Kind::VOID:
      case Expr::Kind::BOOLEAN_LITERAL:
      case Expr::Kind::INTEGER_LITERAL:
      case Expr::Kind::FLOAT_LITERAL:
      case Expr::Kind::SELF:
      case Expr::Kind::SUPER:
        break;

      case Expr::Kind::CALL: {
        auto callExpr = static_cast<ApplyFnOp*>(e);
        visitExpr(callExpr->function, flow);
        for (auto arg : callExpr->args) {
          FlowState argFlow(flow.numLocalVars(), flow.numMemberVars());
          visitExpr(arg, argFlow);
          flow.localVarsRead |= argFlow.localVarsRead;
          flow.localVarsSet |= argFlow.localVarsSet;
          flow.memberVarsSet |= argFlow.memberVarsSet;
        }
        break;
      }

      case Expr::Kind::FUNCTION_REF:
      case Expr::Kind::TYPE_REF:
        break;

      case Expr::Kind::VAR_REF: {
        auto dref = static_cast<DefnRef*>(e);
        auto vd = cast<ValueDefn>(dref->defn);
        visitExpr(dref->stem, flow);
        if (vd->definedIn() == _currentMethod) {
          if (!vd->init() && !flow.localVarsSet[vd->fieldIndex()]) {
            diag.error(e) << "Use of uninitialized variable '" << vd->name() << "'.";
            diag.info(vd->location()) << "Defined here.";
          }
          flow.localVarsRead[vd->fieldIndex()] = true;
        }
        break;
      }

      case Expr::Kind::FUNCTION_REF_OVERLOAD:
      case Expr::Kind::TYPE_REF_OVERLOAD:
        assert(false && "Unresolved ambiguity");

      case Expr::Kind::BLOCK: {
        auto block = static_cast<BlockStmt*>(e);
        for (auto st : block->stmts) {
          visitExpr(st, flow);
        }
        visitExpr(block->result, flow);
        break;
      }

      case Expr::Kind::LOCAL_VAR: {
        auto st = static_cast<LocalVarStmt*>(e);
        auto vd = cast<ValueDefn>(st->defn);
        if (vd->init()) {
          visitExpr(vd->init(), flow);
        }
        break;
      }

      case Expr::Kind::IF: {
        auto stmt = static_cast<IfStmt*>(e);
        visitExpr(stmt->test, flow);
        FlowState thenFlow(flow.numLocalVars(), flow.numMemberVars());
        FlowState elseFlow(flow.numLocalVars(), flow.numMemberVars());
        visitExpr(stmt->thenBlock, thenFlow);
        visitExpr(stmt->elseBlock, elseFlow);
        thenFlow.localVarsSet &= elseFlow.localVarsSet;
        thenFlow.localVarsRead |= elseFlow.localVarsRead;
        thenFlow.memberVarsSet &= elseFlow.memberVarsSet;
        flow.localVarsRead |= thenFlow.localVarsRead;
        flow.localVarsSet |= thenFlow.localVarsSet;
        flow.memberVarsSet |= thenFlow.memberVarsSet;
        break;
      }

      case Expr::Kind::WHILE: {
        auto stmt = static_cast<WhileStmt*>(e);
        visitExpr(stmt->test, flow);
        visitExpr(stmt->body, flow);
        assert(false && "Implement");
        break;
      }

      case Expr::Kind::RETURN: {
        auto ret = static_cast<UnaryOp*>(e);
        visitExpr(ret->arg, flow);
        break;
      }

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
        auto op = static_cast<BinaryOp*>(e);
        visitExpr(op->args[0], flow);
        visitExpr(op->args[1], flow);
        break;
      }

      case Expr::Kind::NEGATE:
      case Expr::Kind::COMPLEMENT: {
        auto op = static_cast<UnaryOp*>(e);
        visitExpr(op->arg, flow);
        break;
      }

      case Expr::Kind::ASSIGN: {
        auto op = static_cast<BinaryOp*>(e);
        visitExpr(op->args[1], flow);
        auto lval = op->args[0];
        if (lval->kind == Expr::Kind::VAR_REF) {
          auto dref = static_cast<DefnRef*>(lval);
          visitExpr(dref->stem, flow);
          auto vd = cast<ValueDefn>(dref->defn);
          if (dref->stem && dref->stem->kind == Expr::Kind::SELF) {
            assert(vd->isMember());
            flow.memberVarsSet[vd->fieldIndex()] = true;
          } else if (vd->definedIn() == _currentMethod) {
            flow.localVarsSet[vd->fieldIndex()] = true;
          }
        } else {
          assert(false && "Implement");
        }
        break;
      }

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(e->kind);
        assert(false && "Invalid expression kind");
    }
  }
}
