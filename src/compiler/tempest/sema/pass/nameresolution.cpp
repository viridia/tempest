#include "tempest/ast/defn.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;

  void NameResolutionPass::run() {
    while (moreSources()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (moreImportSources()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void NameResolutionPass::process(Module* mod) {
    // diag.info() << "Resolving imports: " << mod->name();
    _alloc = &mod->semaAlloc();
    resolveImports(mod);
    ModuleScope scope(nullptr, mod);
    visitList(&scope, mod->members());
    // createMembers(modAst->members, mod, mod->members(), mod->memberScope());
  }

  void NameResolutionPass::resolveImports(Module* mod) {
    auto modAst = static_cast<const ast::Module*>(mod->ast());
    for (auto node : modAst->imports) {
      auto imp = static_cast<const ast::Import*>(node);
      Module* importMod;
      if (imp->relative == 0) {
        importMod = _cu.importMgr().loadModule(imp->path);
      } else {
        importMod = _cu.importMgr().loadModuleRelative(
            imp->location, mod->name(), imp->relative, imp->path);
      }
      if (importMod) {
        for (auto node : imp->members) {
          StringRef importName;
          StringRef asName;
          if (node->kind == ast::Node::Kind::KEYWORD_ARG) {
            auto kw = static_cast<const ast::KeywordArg*>(node);
            asName = kw->name;
            node = kw->arg;
          }

          if (node->kind == ast::Node::Kind::IDENT) {
            auto ident = static_cast<const ast::Ident*>(node);
            importName = ident->name;
          }

          if (asName.empty()) {
            asName = importName;
          }

          ModuleScope impScope(nullptr, importMod);
          NameLookupResult lookupResult;
          impScope.lookup(importName, lookupResult);
          if (lookupResult.empty()) {
            diag.error(imp) << "Imported symbol '" << asName << "' not found in module.";
          } else if (imp->kind == ast::Node::Kind::EXPORT) {
            Location existing;
            if (mod->exportScope()->exists(asName, existing)) {
              diag.error(imp) << "Symbol '" << asName << "' already exported.";
              diag.info(existing) << "From here.";
            } else {
              for (auto member : lookupResult) {
                mod->exportScope()->addMember(asName, member);
              }
            }
          } else {
            Location existing;
            if (mod->memberScope()->exists(asName, existing)) {
              diag.error(imp) << "Imported symbol '" << asName << "' already defined.";
              diag.info(existing) << "Defined here.";
            } else {
              for (auto member : lookupResult) {
                mod->memberScope()->addMember(asName, member);
              }
            }
          }
        }
      }
    }
  }

  void NameResolutionPass::visitList(LookupScope* scope, DefnArray members) {
    for (auto defn : members) {
      switch (defn->kind) {
        case Member::Kind::TYPE: {
          visitTypeDefn(scope, static_cast<TypeDefn*>(defn));
          break;
        }

        case Member::Kind::FUNCTION: {
          visitFunctionDefn(scope, static_cast<FunctionDefn*>(defn));
          break;
        }

        case Member::Kind::CONST_DEF:
        case Member::Kind::LET_DEF: {
          visitValueDefn(scope, static_cast<ValueDefn*>(defn));
          break;
        }

        default:
          assert(false && "Shouldn't get here, bad member type.");
          break;
      }
    }
  }

  void NameResolutionPass::visitTypeDefn(LookupScope* scope, TypeDefn* td) {
    // self.processAttributes(defn)
    // savedSubject = self.nameLookup.setSubject(typeDefn)
    // self.nameLookup.scopeStack.push(typeDefn.getTypeParamScope())
    // resolverequirements.ResolveRequirements(
    //     self.errorReporter, self.resolveExprs, self.resolveTypes, typeDefn).run()
    // typeDefn.setFriends(self.visitList(typeDefn.getFriends()))
    if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
    // if isinstance(typeDefn.getType(), graph.Composite):
    //   self.resolveClassBases(typeDefn.getType())
      TypeDefnScope tdScope(scope, td);
      visitList(&tdScope, td->members());
    }
    // else:
    // self.visitDefn(typeDefn) # Visits members and attrs
    // self.nameLookup.setSubject(savedSubject)
  }

  void NameResolutionPass::visitFunctionDefn(LookupScope* scope, FunctionDefn* fd) {
    // savedSubject = self.nameLookup.setSubject(func)
    // if func.hasTypeParamScope():
    //   self.nameLookup.scopeStack.push(func.getTypeParamScope())
    // resolverequirements.ResolveRequirements(
    //     self.errorReporter, self.resolveExprs, self.resolveTypes, func).run()

    // if func.astReturnType:
    //   returnType = self.resolveTypes.visit(func.astReturnType)
    //   func.getMutableType().setReturnType(returnType)
    // else:
    //   func.getMutableType().setReturnType(primitivetypes.VOID)

    // self.processParamList(func, func.getParams())
    // if func.astBody:
    //   self.nameLookup.scopeStack.push(func.getParamScope())
    //   func.setBody(self.resolveExprs.visit(func.astBody))
    //   self.nameLookup.scopeStack.pop()

    // self.visitDefn(func) # Visits members and attrs
    // if func.hasTypeParamScope():
    //   self.nameLookup.scopeStack.pop()
    // self.nameLookup.setSubject(savedSubject)

    // if len(func.getParams()) > 0:
    //   paramTypes = [param.getType() for param in func.getParams()]
    //   func.getMutableType().setParamTypes(paramTypes)

    // funcType = func.getMutableType()
    // assert func.hasDefinedIn(), func
    // if defns.isGlobalDefn(func):
    //   if func.isConstructor():
    //     self.errorAt(func,
    //         "Constructors cannot be declared at global scope.", func.getName())
    //   elif func.hasSelfType():
    //     self.errorAt(func,
    //         "Global function '(0)' cannot declare a 'self' parameter.", func.getName())
    //     func.clearSelfType()
    // elif defns.isStaticDefn(func):
    //   if func.hasSelfType():
    //     self.errorAt(func,
    //         "Static function '(0)' cannot declare a 'self' parameter.", func.getName())
    //     funcType.clearSelfType()
    // else:
    //   if not func.hasSelfType():
    //     enc = defns.getEnclosingTypeDefn(func)
    //     assert enc, debug.format(func)
    //     func.setSelfType(enc.getType())

    // if func.getName() == 'new' and not func.isStatic():
    //   assert func.hasSelfType()
  }

  void NameResolutionPass::visitValueDefn(LookupScope* scope, ValueDefn* vd) {
    auto vdAst = static_cast<const ast::ValueDefn*>(vd->ast());
    if (vdAst->type) {
      vd->setType(resolveType(scope, vdAst->type));
    }
    // if vdef.astInit:
    //   if vdef.hasType() and isinstance(vdef.getType(), graph.Enum):
    //     self.nameLookup.scopeStack.push(vdef.getType().getDefn().getMemberScope())
    //     vdef.setInit(self.resolveExprs.visit(vdef.astInit))
    //     self.nameLookup.scopeStack.pop()
    //   else:
    //     init = self.resolveExprs.visit(vdef.astInit)
    //     if not self.isErrorExpr(init):
    //       vdef.setInit(init)
    // self.visitDefn(vdef) # Visits members and attrs
    // return vdef
  }

  Type* NameResolutionPass::resolveType(LookupScope* scope, const ast::Node* node) {
    switch (node->kind) {
      case ast::Node::Kind::IDENT: {
        auto ident = static_cast<const ast::Ident*>(node);
        assert(false && "Implement");
        break;
      }

      case ast::Node::Kind::MEMBER: {
        auto memberRef = static_cast<const ast::MemberRef*>(node);
        assert(false && "Implement");
        break;
      }

      case ast::Node::Kind::SPECIALIZE: {
        auto spec = static_cast<const ast::Oper*>(node);
        assert(false && "Implement");
        break;
      }

      case ast::Node::Kind::BIT_OR: {
        llvm::SmallVector<Type*, 8> unionMembers;
        std::function<void(const ast::Node*)> flatUnion;
        flatUnion = [this, scope, &unionMembers, &flatUnion](const ast::Node* in) -> void {
          if (in->kind == ast::Node::Kind::BIT_OR) {
            auto unionOp = static_cast<const ast::Oper*>(in);
            flatUnion(unionOp->operands[0]);
            flatUnion(unionOp->operands[1]);
          } else {
            auto t = resolveType(scope, in);
            if (t) {
              unionMembers.push_back(t);
            }
          }
        };
        flatUnion(node);
        // TODO: Do we need to ensure uniqueness here?
        return _cu.types().createUnionType(unionMembers);
      }

      case ast::Node::Kind::BUILTIN_TYPE: {
        auto bt = static_cast<const ast::BuiltinType*>(node);
        switch (bt->type) {
          case ast::BuiltinType::VOID:
            return &VoidType::VOID;
          case ast::BuiltinType::BOOL:
            return &BooleanType::BOOL;
          case ast::BuiltinType::CHAR:
            return &IntegerType::CHAR;
          case ast::BuiltinType::I8:
            return &IntegerType::I8;
          case ast::BuiltinType::I16:
            return &IntegerType::I16;
          case ast::BuiltinType::I32:
            return &IntegerType::I32;
          case ast::BuiltinType::I64:
            return &IntegerType::I64;
          case ast::BuiltinType::INT:
            // TODO: I32 or I64
            return &IntegerType::I64;
          case ast::BuiltinType::U8:
            return &IntegerType::U8;
          case ast::BuiltinType::U16:
            return &IntegerType::U16;
          case ast::BuiltinType::U32:
            return &IntegerType::U32;
          case ast::BuiltinType::U64:
            return &IntegerType::U64;
          case ast::BuiltinType::UINT:
            // TODO: U32 or U64
            return &IntegerType::U64;
          case ast::BuiltinType::F32:
            return &FloatType::F32;
          case ast::BuiltinType::F64:
            return &FloatType::F64;
        }
      }
    }
  }
}
