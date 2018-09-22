#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "tempest/sema/graph/expr.hpp"
// #include "tempest/sema/graph/expr_literal.hpp"
// #include "tempest/sema/graph/expr_op.hpp"
// #include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/pass/expandspecialization.hpp"
#include "tempest/sema/transform/transform.hpp"
#include "tempest/sema/transform/visitor.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace tempest::sema::graph;
  using tempest::sema::transform::ApplySpecialization;

  class FindSpecializationsVisitor final : public transform::ExprVisitor {
  public:
    FindSpecializationsVisitor(CompilationUnit& cu, Env& env)
      : _cu(cu)
      , _env(env)
    {}

    Expr* visit(Expr* e) override {
      if (e == nullptr) {
        return e;
      }

      switch (e->kind) {
        case Expr::Kind::FUNCTION_REF:
        case Expr::Kind::TYPE_REF: {
          auto dref = static_cast<DefnRef*>(e);
          if (dref->defn->kind == Member::Kind::SPECIALIZED) {
            auto sp = static_cast<SpecializedDefn*>(dref->defn);
            llvm::SmallVector<const Type*, 8> typeArgs;
            ApplySpecialization transform(_env.args);
            transform.transformArray(typeArgs, sp->typeArgs());
            for (auto ta : typeArgs) {
              assert(ta->kind != Type::Kind::TYPE_VAR);
              assert(ta->kind != Type::Kind::INFERRED);
            }
            // Make a new specialization with concrete type arguments, and add it to the list
            // of reachable specializations in the compilation unit.
            sp = _cu.spec().specialize(cast<GenericDefn>(sp->generic()), typeArgs);
            _cu.spec().addConcreteSpec(sp);
            dref->defn = sp;
          }
          return transform::ExprVisitor::visit(e);
        }

        default:
          return transform::ExprVisitor::visit(e);
      }
    }

  private:
    CompilationUnit& _cu;
    Env& _env;
  };

  void ExpandSpecializationPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
    while (_specializationsProcessed < _cu.spec().concreteSpecs().size()) {
      auto sp = _cu.spec().concreteSpecs()[_specializationsProcessed++];
      auto generic = cast<GenericDefn>(sp->generic());
      Env env;
      env.params = generic->allTypeParams();
      env.args.assign(sp->typeArgs().begin(), sp->typeArgs().end());
      visitDefn(generic, env);
    }
  }

  void ExpandSpecializationPass::process(Module* mod) {
    // begin(mod);
    Env empty;
    visitList(mod->members(), empty);
  }

  // Definitions

  void ExpandSpecializationPass::visitList(DefnArray members, Env& env) {
    for (auto defn : members) {
      visitDefn(defn, env);
    }
  }

  void ExpandSpecializationPass::visitDefn(Defn* d, Env& env) {
    switch (d->kind) {
      case Member::Kind::TYPE: {
        visitTypeDefn(static_cast<TypeDefn*>(d), env);
        break;
      }

      case Member::Kind::FUNCTION: {
        visitFunctionDefn(static_cast<FunctionDefn*>(d), env);
        break;
      }

      case Member::Kind::CONST_DEF:
      case Member::Kind::LET_DEF: {
        visitValueDefn(static_cast<ValueDefn*>(d), env);
        break;
      }

      default:
        assert(false && "Shouldn't get here, bad member type.");
        break;
    }
  }

  void ExpandSpecializationPass::visitTypeDefn(TypeDefn* td, Env& env) {
    // Don't visit templates.
    if (td->allTypeParams().size() > env.args.size()) {
      return;
    }

    visitAttributes(td, env);
    visitList(td->members(), env);
  }

  void ExpandSpecializationPass::visitFunctionDefn(FunctionDefn* fd, Env& env) {
    // Don't visit templates.
    if (fd->allTypeParams().size() > env.args.size()) {
      return;
    }

    visitAttributes(fd, env);
    for (auto param : fd->params()) {
      visitAttributes(param, env);
      if (param->init()) {
        visitExpr(param->init(), env);
      }
    }

    if (fd->body()) {
      visitExpr(fd->body(), env);
    }
  }

  void ExpandSpecializationPass::visitValueDefn(ValueDefn* vd, Env& env) {
    visitAttributes(vd, env);
    if (vd->init()) {
      visitExpr(vd->init(), env);
    }
  }

  void ExpandSpecializationPass::visitAttributes(Defn* defn, Env& env) {
    for (auto attr : defn->attributes()) {
      visitExpr(attr, env);
    }
  }

  void ExpandSpecializationPass::visitExpr(Expr* expr, Env& env) {
    FindSpecializationsVisitor visitor(_cu, env);
    visitor.visit(expr);
  }
}
