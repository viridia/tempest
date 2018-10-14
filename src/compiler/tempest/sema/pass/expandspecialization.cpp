#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/expr_lowered.hpp"
#include "tempest/sema/pass/expandspecialization.hpp"
#include "tempest/sema/transform/mapenv.hpp"
#include "tempest/sema/transform/visitor.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace tempest::sema::graph;
  using namespace tempest::gen;
  using tempest::sema::transform::MapEnvTransform;

  /** Add references to output symbols where needed. */
  class GenSymVisitor final : public transform::ExprVisitor {
  public:
    GenSymVisitor(CompilationUnit& cu) : _cu(cu) {}

    Expr* visit(Expr* e) override {
      if (e == nullptr) {
        return e;
      }

      switch (e->kind) {
        case Expr::Kind::ALLOC_OBJ: {
          Env empty;
          auto sref = static_cast<SymbolRefExpr*>(e);
          auto udt = cast<UserDefinedType>(e->type);
          sref->sym = _cu.symbols().addClass(udt->defn(), empty);
          return transform::ExprVisitor::visit(e);
        }

        default:
          return transform::ExprVisitor::visit(e);
      }
    }

  private:
    CompilationUnit& _cu;
  };

  /** Replace all type variables with the types they are bound to in an environment. */
  class SpecializeTypeTransform final : public transform::UniqueTypeTransform {
  public:
    SpecializeTypeTransform(CompilationUnit& cu, Env& env)
      : transform::UniqueTypeTransform(cu.types(), cu.spec())
      , _env(env)
    {}

    const Type* transformTypeVar(const TypeVar* in) override final {
      return _env.args[static_cast<const TypeVar*>(in)->index()];
    }

  private:
    Env& _env;
  };

  /** Replace all reference to type variables with the bound values in an environment. */
  class SpecializationExprTransform final : public transform::ExprTransform {
  public:
    SpecializationExprTransform(CompilationUnit& cu, Env& env)
      : transform::ExprTransform(_cu.types().alloc())
      , _cu(cu)
      , _env(env)
      , _typeTransform(cu, env)
    {}

    Expr* transform(Expr* e) override {
      if (e == nullptr) {
        return e;
      }

      switch (e->kind) {
        case Expr::Kind::FUNCTION_REF:
        case Expr::Kind::TYPE_REF: {
          auto dref = static_cast<DefnRef*>(e);
          if (dref->defn->kind == Member::Kind::SPECIALIZED) {
            auto sp = static_cast<SpecializedDefn*>(dref->defn);
            auto generic = cast<GenericDefn>(sp->generic());
            auto stem = transform(dref->stem);
            auto type = transformType(dref->type);
            Env newEnv;
            newEnv.params = generic->allTypeParams();
            MapEnvTransform transform(_cu.types(), _cu.spec(), _env);
            transform.transformArray(newEnv.args, sp->typeArgs());
            for (auto ta : newEnv.args) {
              assert(ta->kind != Type::Kind::TYPE_VAR);
              assert(ta->kind != Type::Kind::INFERRED);
            }
            // Make a new specialization with concrete type arguments, and add it to the list
            // of reachable specializations in the compilation unit.
            if (auto fd = dyn_cast<FunctionDefn>(generic)) {
              _cu.symbols().addFunction(fd, newEnv);
            } else if (auto td = dyn_cast<TypeDefn>(generic)) {
              if (td->type()->kind == Type::Kind::CLASS) {
                _cu.symbols().addClass(td, newEnv);
              } else if (td->type()->kind == Type::Kind::INTERFACE) {
                _cu.symbols().addInterface(td, newEnv);
              }
            }
            auto newSp = _cu.spec().specialize(generic, newEnv.args);
            if (newSp != sp || stem != dref->stem || type != dref->type) {
              return new (alloc()) DefnRef(dref->kind, dref->location, newSp, stem, type);
            }
            return dref;
          }
          return transform::ExprTransform::transform(e);
        }

        // LET_DEF

        case Expr::Kind::ALLOC_OBJ: {
          auto sref = static_cast<SymbolRefExpr*>(e);
          auto udt = cast<UserDefinedType>(e->type);
          sref->sym = _cu.symbols().addClass(udt->defn(), _env);
          return transform::ExprTransform::transform(e);
        }

        default:
          return transform::ExprTransform::transform(e);
      }
    }

    virtual Type* transformType(Type* type) override final {
      return const_cast<Type*>(_typeTransform.transform(type));
    }

  private:
    tempest::support::BumpPtrAllocator& alloc() { return _cu.types().alloc(); }

    CompilationUnit& _cu;
    Env& _env;
    SpecializeTypeTransform _typeTransform;
  };

  void ExpandSpecializationPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
    while (_symbolsProcessed < _cu.symbols().list().size()) {
      auto sym = _cu.symbols().list()[_symbolsProcessed++];
      if (auto fsym = dyn_cast<FunctionSym>(sym)) {
        visitFunctionSym(fsym);
      } else if (auto csym = dyn_cast<ClassDescriptorSym>(sym)) {
        visitClassDescriptorSym(csym);
      } else if (auto isym = dyn_cast<InterfaceDescriptorSym>(sym)) {
        visitInterfaceDescriptorSym(isym);
      } else if (auto vsym = dyn_cast<GlobalVarSym>(sym)) {
        visitGlobalVarSym(vsym);
      }
    }
  }

  void ExpandSpecializationPass::process(Module* mod) {
    Env empty;
    // Add all top-level members as output symbols.
    for (auto d : mod->members()) {
      switch (d->kind) {
        case Member::Kind::TYPE: {
          auto td = static_cast<TypeDefn*>(d);
          // Don't include templates.
          if (td->allTypeParams().empty()) {
            if (td->type()->kind == Type::Kind::CLASS) {
              _cu.symbols().addClass(td, empty);
            } else if (td->type()->kind == Type::Kind::INTERFACE) {
              _cu.symbols().addInterface(td, empty);
            }
          }
          break;
        }

        case Member::Kind::FUNCTION: {
          auto fd = static_cast<FunctionDefn*>(d);
          // Don't include templates.
          if (fd->allTypeParams().empty()) {
            auto fsym = _cu.symbols().addFunction(fd, empty);
            GenSymVisitor visitor(_cu);
            visitor.visit(fd->body());
            fsym->body = fd->body();
          }
          break;
        }

        case Member::Kind::CONST_DEF:
        case Member::Kind::LET_DEF: {
          auto vd = static_cast<ValueDefn*>(d);
          _cu.symbols().addGlobalVar(vd, empty);
          break;
        }

        default:
          assert(false && "Shouldn't get here, bad member type.");
          break;
      }
    }
  }

  void ExpandSpecializationPass::visitFunctionSym(FunctionSym* fsym) {
    Env env;
    env.params = fsym->function->allTypeParams();
    env.args.assign(fsym->typeArgs.begin(), fsym->typeArgs.end());

    auto fd = fsym->function;
    if (fd->body()) {
      assert(env.args.size() == fd->allTypeParams().size());
      fsym->body = expandExpr(fd->body(), env);
    }
  }

  void ExpandSpecializationPass::visitClassDescriptorSym(ClassDescriptorSym* csym) {
    Env env;
    env.params = csym->typeDefn->allTypeParams();
    env.args.assign(csym->typeArgs.begin(), csym->typeArgs.end());

    for (auto member : csym->typeDefn->members()) {
      if (auto fd = dyn_cast<FunctionDefn>(member)) {
        // Static
        // Final or overloaded
        // Interface / Class
        if (!fd->isStatic() && fd->allTypeParams().size() == csym->typeArgs.size()) {
          auto fsym = _cu.symbols().addFunction(fd, env);
          (void)fsym;
        }
      }
    }
  }

  void ExpandSpecializationPass::visitInterfaceDescriptorSym(InterfaceDescriptorSym* isym) {
    // Env env;
    // env.params = isym->typeDefn->allTypeParams();
    // env.args.assign(isym->typeArgs.begin(), isym->typeArgs.end());

    // for (auto member : isym->typeDefn->members()) {
    //   if (auto fd = dyn_cast<FunctionDefn>(member)) {
    //     if (!fd->isStatic()) {
    //     }
    //   }
    // }
  }

  void ExpandSpecializationPass::visitGlobalVarSym(GlobalVarSym* gsym) {
    Env env;
    // env.params = vsym->varDefn->allTypeParams();
    env.args.assign(gsym->typeArgs.begin(), gsym->typeArgs.end());
  }

  // void ExpandSpecializationPass::visitTypeDefn(TypeDefn* td, Env& env) {
  //   // Don't visit templates.
  //   visitAttributes(td, env);
  // }

  bool ExpandSpecializationPass::expandExprList(
    llvm::SmallVectorImpl<Expr*>& out, ArrayRef<Expr*> exprs, Env& env) {
    bool changed = false;
    for (auto e : exprs) {
      auto ee = expandExpr(e, env);
      out.push_back(ee);
      changed |= (ee != e);
    }
    return changed;
  }

  Expr* ExpandSpecializationPass::expandExpr(Expr* expr, Env& env) {
    SpecializationExprTransform transform(_cu, env);
    return transform.transform(expr);
  }
}
