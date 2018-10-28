#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/pass/dataflow.hpp"
#include "tempest/sema/transform/visitor.hpp"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using tempest::sema::transform::ExprVisitor;
  using namespace llvm;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;

  class FindSuperCtorCall : public ExprVisitor {
  public:
    Expr* superCall = nullptr;
    bool firstSt = true;

    Expr* visit(Expr* e) {
      if (e == nullptr) {
        return e;
      }

      switch (e->kind) {
        case Expr::Kind::ASSIGN:
        case Expr::Kind::CALL: {
          auto callExpr = static_cast<ApplyFnOp*>(e);
          auto callable = callExpr->function;
          if (callExpr->flavor == ApplyFnOp::SUPER && callable->kind == Expr::Kind::FUNCTION_REF) {
            auto dref = static_cast<DefnRef*>(callable);
            auto fnref = cast<FunctionDefn>(unwrapSpecialization(dref->defn));
            auto selfParam = dref->stem;
            if (selfParam && selfParam->kind == Expr::Kind::SELF && fnref->isConstructor()) {
              if (superCall) {
                diag.error(e) << "Only one call to superclass constructor is permitted.";
              } else {
                superCall = e;
                if (!firstSt) {
                  diag.error(e) <<
                      "Call to superclass constructor must come before other statements.";
                }
              }
            }
          }
          e = ExprVisitor::visit(e);
          firstSt = false;
          return e;
        }

        default:
          return ExprVisitor::visit(e);
      }
    }
  };

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

        case Member::Kind::VAR_DEF: {
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

      // Make sure there are no unused / uninitialized local variables.
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

      // Make sure that constructors initialize all variables and call the superclass ctor.
      if (fd->isConstructor()) {
        auto td = cast<TypeDefn>(fd->definedIn());
        llvm::SmallVector<Expr*, 8> initStmts;
        FindSuperCtorCall findSuper;
        findSuper.visit(fd->body());

        if (!findSuper.superCall) {
          assert(td->extends().size() == 1);
          ArrayRef<const Type*> typeArgs;
          auto baseCls = unwrapSpecialization(td->extends()[0], typeArgs);
          MemberLookupResult ctors;
          cast<TypeDefn>(baseCls)->memberScope()->lookup("new", ctors, nullptr);
          FunctionDefn* defaultCtor = nullptr;
          if (ctors.empty()) {
            diag.error(td) << "Base class lacks any constructor definitions";
          } else {
            for (auto m : ctors) {
              if (auto ctor = dyn_cast<FunctionDefn>(m.member)) {
                if (ctor->params().size() == 0) {
                  defaultCtor = ctor;
                  break;
                }
              }
            }

            if (td->isFlex()) {
              // Flex alloc types don't need a call to supertype.
            } else if (!defaultCtor) {
              diag.error(td) << "Base class has no default constructor";
            } else {
              // Inset automatically-generated superclass contructor call.
              Member* ctor = defaultCtor;
              if (typeArgs.size() > 0) {
                ctor = _cu.spec().specialize(defaultCtor, typeArgs);
              }
              // TODO: Probably need to upcast self.
              auto dref = new (*_alloc) DefnRef(
                  Expr::Kind::FUNCTION_REF, fd->location(), ctor, td->implicitSelf());
              auto call = new (*_alloc) ApplyFnOp(Expr::Kind::CALL, fd->location(), dref, {});
              call->flavor = ApplyFnOp::SUPER;
              initStmts.push_back(call);
            }
          }
        }

        for (auto m : td->members()) {
          if (auto vd = dyn_cast<ValueDefn>(m)) {
            if (!vd->isStatic()) {
              if (!flow.memberVarsSet[vd->fieldIndex()]) {
                if (vd->init()) {
                  if (!(vd->isConstant() && vd->isConstantInit())) {
                    auto dref = new (*_alloc) DefnRef(
                        Expr::Kind::VAR_REF, fd->location(), vd, td->implicitSelf());
                    initStmts.push_back(
                        new (*_alloc) BinaryOp(Expr::Kind::ASSIGN, dref, vd->init()));
                  }
                } else {
                  diag.error(fd) << "Field not initialized: '" << vd->name() << "'.";
                }
              }
            }
          }
        }

        if (!initStmts.empty()) {
          // Append 'super' call and field initializations
          if (auto blk = dyn_cast<BlockStmt>(fd->body())) {
            // Put the initialization statements between the call to super() and the rest
            // of the block. If there's no call to super(), but them at the beginning.
            auto insertionPt = blk->stmts.end();
            if (findSuper.superCall) {
              insertionPt = std::find(blk->stmts.begin(), blk->stmts.end(), findSuper.superCall);
              if (insertionPt == blk->stmts.end()) {
                diag.error(fd) << "Can't find location of call to super().";
              } else {
                insertionPt++;
              }
            }

            initStmts.insert(initStmts.begin(), blk->stmts.begin(), insertionPt);
            initStmts.append(insertionPt, blk->stmts.end());
            blk->stmts = _alloc->copyOf(initStmts);
          } else if (fd->body() == findSuper.superCall) {
            initStmts.insert(initStmts.begin(), findSuper.superCall);
            fd->setBody(new (*_alloc) BlockStmt(
                fd->body()->location, initStmts, nullptr, fd->body()->type));
          } else if (!findSuper.superCall) {
            fd->setBody(new (*_alloc) BlockStmt(
                fd->body()->location, initStmts, fd->body(), fd->body()->type));
          } else {
            diag.error(fd) << "Can't find location of call to super().";
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
      case Expr::Kind::ALLOC_OBJ:
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
      case Expr::Kind::TYPE_REF: {
        auto dref = static_cast<DefnRef*>(e);
        visitExpr(dref->stem, flow);
        break;
      }

      case Expr::Kind::VAR_REF: {
        auto dref = static_cast<DefnRef*>(e);
        auto vd = cast<ValueDefn>(dref->defn);
        visitExpr(dref->stem, flow);
        if (vd->isLocal()) {
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
        } else if (lval->kind == Expr::Kind::SELF) {
          if (_currentMethod->isConstructor() &&
              cast<TypeDefn>(_currentMethod->definedIn())->isFlex()) {
            // Assignment to self is legal here.
          } else {
            diag.debug() << "Assignment to 'self' is not permitted.";
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
