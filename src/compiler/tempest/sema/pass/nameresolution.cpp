#include "tempest/ast/defn.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/literal.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/eval/evalconst.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/names/closestname.hpp"
#include "tempest/sema/names/createnameref.hpp"
#include "tempest/sema/names/membernamelookup.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/transform/mapenv.hpp"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ConvertUTF.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;
  using tempest::sema::transform::MapEnvTransform;

  namespace {
    // Given a (possibly specialized) type definition, return what kind of type it is.
    Type::Kind typeKindOf(Member* m) {
      m = unwrapSpecialization(m);

      if (auto td = dyn_cast_or_null<TypeDefn>(m)) {
        return td->type()->kind;
      }
      return Type::Kind::INVALID;
    }
  }

  void NameResolutionPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void NameResolutionPass::process(Module* mod) {
    _alloc = &mod->semaAlloc();
    resolveImports(mod);
    buildExtensionMap(mod);
    ModuleScope scope(nullptr, mod);
    visitList(&scope, mod->members());
  }

  void NameResolutionPass::resolveImports(Module* mod) {
    for (auto node : mod->ast()->imports) {
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

          MemberLookupResult lookupResult;
          importMod->exportScope()->lookup(importName, lookupResult, nullptr);
          if (lookupResult.empty()) {
            diag.error(imp) << "No exported symbol '" << asName << "' found in module.";
          } else if (imp->kind == ast::Node::Kind::EXPORT) {
            Location existing;
            if (mod->exportScope()->exists(asName, existing)) {
              diag.error(imp) << "Symbol '" << asName << "' already exported.";
              diag.info(existing) << "From here.";
            } else {
              for (auto member : lookupResult) {
                mod->exportScope()->addMember(asName, member.member);
              }
            }
          } else {
            Location existing;
            if (mod->memberScope()->exists(asName, existing)) {
              diag.error(imp) << "Imported symbol '" << asName << "' already defined.";
              diag.info(existing) << "Defined here.";
            } else {
              for (auto member : lookupResult) {
                mod->memberScope()->addMember(asName, member.member);
              }
            }
          }
        }
      }
    }
  }

  void NameResolutionPass::buildExtensionMap(Module* mod) {
    auto extensionCallback = [mod](Member* member, llvm::StringRef name) {
      // See if it's an extension type
      if (auto td = dyn_cast<TypeDefn>(member)) {
        if (td->type()->kind == Type::Kind::EXTENSION) {
          // Find all type definitions that have the same name
          NameLookupResult results;
          mod->memberScope()->lookupName(name, results);
          size_t numTargets = 0;
          for (auto nlr : results) {
            if (auto tdTarget = dyn_cast<TypeDefn>(nlr)) {
              if (tdTarget->type()->kind != Type::Kind::EXTENSION) {
                mod->extensions()->add(tdTarget, td);
                numTargets += 1;
              }
            } else {
              diag.error(source::Location(mod->source(), 0, 0, 0, 0)) <<
                  "Type extension target '" << name << "' is not a type.";
              numTargets += 1;
              break;
            }
          }
          if (numTargets == 0) {
            diag.error(source::Location(mod->source(), 0, 0, 0, 0)) <<
                "no valid targets found for type extension '" << name << "'.";
          }
        }
      }
    };
    mod->memberScope()->forAllMembers(extensionCallback);
  }

  void NameResolutionPass::visitList(LookupScope* scope, DefnArray members) {
    for (auto defn : members) {
      auto enclosingType = dyn_cast_or_null<TypeDefn>(defn->definedIn());
      auto enclosingKind = enclosingType ? enclosingType->type()->kind : Type::Kind::VOID;

      if (defn->visibility() == Visibility::PRIVATE && !enclosingType) {
        diag.error(defn) << "Only class or struct members can have 'private' visibility.";
      } else if (defn->visibility() == Visibility::PROTECTED && !enclosingType) {
        diag.error(defn) << "Only class or struct members can have 'protected' visibility.";
      } else if (enclosingType && defn->visibility() != Visibility::PUBLIC) {
        if (enclosingKind == Type::Kind::INTERFACE) {
          diag.error(defn) << "Interface members cannot be 'private' or 'protected'.";
        } else if (enclosingKind == Type::Kind::TRAIT) {
          diag.error(defn) << "Trait members cannot be 'private' or 'protected'.";
        }
      }

      switch (defn->kind) {
        case Member::Kind::TYPE: {
          visitTypeDefn(scope, static_cast<TypeDefn*>(defn));
          break;
        }

        case Member::Kind::FUNCTION: {
          visitFunctionDefn(scope, static_cast<FunctionDefn*>(defn));
          break;
        }

        case Member::Kind::VAR_DEF: {
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
    visitAttributes(scope, td, td->ast());
    visitTypeParams(scope, td);
    // typeDefn.setFriends(self.visitList(typeDefn.getFriends()))
    if (td->type()->kind == Type::Kind::ALIAS) {
      TypeParamScope tpScope(scope, td); // Scope used in resolving param types only.
      td->setAliasTarget(resolveType(&tpScope, td->ast()->extends[0], true));
    } else if (td->type()->kind == Type::Kind::ENUM) {
      visitEnumDefn(scope, td);
    } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(scope, td);
    }
  }

  void NameResolutionPass::visitCompositeDefn(LookupScope* scope, TypeDefn* td) {
    if (td->isFinal()) {
      if (td->isAbstract()) {
        diag.error(td) << "Type definition cannot be both abtract and final.";
      } else if (td->type()->kind == Type::Kind::INTERFACE) {
        diag.error(td) << "Interfaces cannot be final.";
      } else if (td->type()->kind == Type::Kind::TRAIT) {
        diag.error(td) << "Traits cannot be final.";
      } else if (td->type()->kind == Type::Kind::EXTENSION) {
        diag.error(td) << "Extensions cannot be declared final.";
      }
    } else if (td->isAbstract()) {
      if (td->type()->kind != Type::Kind::CLASS) {
        diag.error(td) << "Only classes can be declared abstract.";
      }
    }

    if (td->isOverride()) {
      diag.error(td) << "Type definition cannot be declared as override.";
    }

    if (td->isStatic()) {
      auto subject = scope->subject();
      if (!subject) {
        diag.error(td) << "Only inner types may be declared as 'static'.";
      } else if (subject->kind == Member::Kind::TYPE
          && cast<TypeDefn>(subject)->type()->kind == Type::Kind::STRUCT) {
        diag.error(td) << "Types defined within a struct type are always static.";
      }
    }

    TypeParamScope tpScope(scope, td); // Scope used in resolving param types only.
    resolveBaseTypes(&tpScope, td);
    td->setImplicitSelf(new (*_alloc) SelfExpr(td->location(), td->type()));

    auto prevNumInstanceVars = _numInstanceVars;
    _numInstanceVars = 0;
    if (td->extends().size() > 0) {
      auto baseDef = unwrapSpecialization(td->extends()[0]);
      _numInstanceVars = cast<TypeDefn>(baseDef)->numInstanceVars();
    }

    auto prevTypeDefn = _typeDefn;
    _typeDefn = td;
    TypeDefnScope tdScope(scope, td, _cu.spec());
    visitList(&tdScope, td->members());

    td->setNumInstanceVars(_numInstanceVars);
    _numInstanceVars = prevNumInstanceVars;
    _typeDefn = prevTypeDefn;

    // Create default/inherited constructors if needed.
    if (td->type()->kind == Type::Kind::CLASS) {
      MemberLookupResult ctors;
      td->memberScope()->lookup("new", ctors, nullptr);
      if (ctors.empty()) {
        // Find inherited constructors
        MemberNameLookup lookup(_cu.spec());
        lookup.lookup("new", td, ctors,
            MemberNameLookup::INHERITED_ONLY | MemberNameLookup::INSTANCE_MEMBERS);

        if (ctors.empty()) {
          diag.error(td) << "Class has no constructor, and base class has no default constructor.";
        }

        // Create a constructor for this class for each inherited base constructor.
        for (auto& ctorResult : ctors) {
          // Actually, we need to check if the base class has constructors...
          // Create default constructor
          auto defaultCtor = new (*_alloc) FunctionDefn(td->location(), "new", td);
          defaultCtor->setConstructor(true);
          defaultCtor->setDefault(true);
          defaultCtor->setSelfType(td->type());
          defaultCtor->allTypeParams().assign(
              td->allTypeParams().begin(), td->allTypeParams().end());
          Env baseEnv;
          auto baseCtor = baseEnv.unwrap(ctorResult.member);

          MapEnvTransform transform(_cu.types(), _cu.spec(), baseEnv.params, baseEnv.args);
          auto baseFn = cast<FunctionDefn>(baseCtor);
          llvm::SmallVector<const Type*, 8> paramTypes;
          llvm::SmallVector<Expr*, 8> superParams;
          llvm::SmallVector<Expr*, 8> ctorStmts;
          for (auto param : baseFn->params()) {
            auto paramType = transform.transform(param->type());
            auto np = new ParameterDefn(td->location(), param->name(), defaultCtor, paramType);
            defaultCtor->params().push_back(np);
            defaultCtor->paramScope()->addMember(np);
            paramTypes.push_back(paramType);
            superParams.push_back(
                new (*_alloc) DefnRef(Expr::Kind::VAR_REF, td->location(), np, nullptr));
          }
          defaultCtor->setType(_cu.types().createFunctionType(&VoidType::VOID, paramTypes, false));
          td->members().insert(td->members().begin(), defaultCtor);
          td->memberScope()->addMember(defaultCtor);
          // TODO: Initialize other variables in this class.

          auto superRef = new (*_alloc) SuperExpr(td->location());
          auto superCall = new (*_alloc) ApplyFnOp(
              Expr::Kind::CALL, td->location(), superRef, _alloc->copyOf(superParams), &VoidType::VOID);
          ctorStmts.push_back(superCall);
          defaultCtor->setBody(
            new (*_alloc) BlockStmt(td->location(), _alloc->copyOf(ctorStmts)));
        }
      }
    }
  }

  void NameResolutionPass::visitEnumDefn(LookupScope* scope, TypeDefn* td) {
    visitAttributes(scope, td, td->ast());
    resolveBaseTypes(scope, td);

    if (td->isStatic()) {
      diag.error(td) << "Enumeration types cannot be declared as 'static'.";
    } else if (td->isAbstract()) {
      diag.error(td) << "Enumeration types cannot be declared as abstract.";
    } else if (td->isOverride()) {
      diag.error(td) << "Enumeration types cannot be overridden.";
    } else if (td->isFinal()) {
      diag.error(td) << "Enumeration types are alwaus final.";
    }

    auto base = cast<IntegerType>(cast<TypeDefn>(td->extends()[0])->type());
    APInt index(base->bits(), 0, !base->isUnsigned());
    TypeDefnScope tdScope(scope, td, _cu.spec());

    for (auto m : td->members()) {
      assert(m->kind == Defn::Kind::ENUM_VAL);
      auto ev = static_cast<ValueDefn*>(m);
      ev->setType(td->type());
      if (ev->ast()->init) {
        auto expr = visitExpr(&tdScope, ev->ast()->init);
        eval::EvalResult result;
        result.failSilentIfNonConst = true;
        if (!eval::evalConstExpr(expr, result) || result.type != eval::EvalResult::INT) {
          if (!result.error) {
            diag.error(expr) << "Enumeration value expression should be a constant integer";
          }
          break;
        }

        if (base->isUnsigned()) {
          if (!result.isUnsigned && result.intResult.isNegative()) {
            diag.error(expr) << "Can't assign a negative value to an unsigned enumeration.";
            break;
          } else if (result.intResult.getActiveBits() > (unsigned) base->bits()) {
            diag.error(expr) << "Constant integer size too large to fit in enum value";
            break;
          }
        } else {
          if (result.intResult.getMinSignedBits() > (unsigned) base->bits()) {
            diag.error(expr) << "Constant integer size too large to fit in enum value";
            break;
          }
        }

        if (base->isUnsigned()) {
          index = result.intResult.zextOrTrunc(base->bits());
        } else {
          index = result.intResult.sextOrTrunc(base->bits());
        }
      }
      ev->setInit(new (*_alloc) IntegerLiteral(
          ev->location(), _alloc->copyOf(index++), base));
    }
  }

  void NameResolutionPass::visitFunctionDefn(LookupScope* scope, FunctionDefn* fd) {
    auto enclosingType = dyn_cast_or_null<TypeDefn>(fd->definedIn());
    auto enclosingKind = enclosingType ? enclosingType->type()->kind : Type::Kind::VOID;

    visitAttributes(scope, fd, fd->ast());

    if (fd->isNative()) {
      if (fd->isAbstract()) {
        diag.error(fd) << "Native functions cannot be abstract.";
      } else if (fd->ast()->body) {
        diag.error(fd) << "Native functions cannot have a function body.";
      }
    } else if (fd->isStatic()) {
      if (fd->isAbstract()) {
        diag.error(fd) << "Static functions cannot be abstract.";
      } else if (!fd->ast()->body) {
        diag.error(fd) << "Static function must have a function body.";
      } else if (enclosingKind == Type::Kind::INTERFACE) {
        diag.error(fd) << "Static function may not be defined within an interface.";
      } else if (fd->isConstructor()) {
        diag.error(fd) << "Constructor functions cannot be declared as 'static'.";
      } else if (fd->isFinal()) {
        diag.error(fd) << "Static functions cannot be declared as 'final'.";
      } else if (fd->isMutableSelf()) {
        diag.error(fd) << "Static functions cannot be declared as mutable self.";
      }
    } else if (fd->isAbstract()) {
      if (fd->ast()->body) {
        diag.error(fd) << "Abstract function cannot have a function body.";
      } else if (fd->isFinal()) {
        diag.error(fd) << "Abstract function cannot be declared as 'final'.";
      } else if (!enclosingType || !enclosingType->isAbstract()) {
        diag.error(fd) << "Abstract function must be defined within an abstract class.";
      } else if (enclosingKind == Type::Kind::INTERFACE) {
        diag.error(fd) << "A function defined within an interface is implicitly abstract.";
      } else if (enclosingKind == Type::Kind::EXTENSION) {
        diag.error(fd) << "A function defined within a type extension cannot be abstract.";
      }
    } else if (fd->intrinsic() != IntrinsicFn::NONE) {
      if (fd->ast()->body) {
        diag.error(fd) << "Intrinsic function cannot have a function body.";
      } else if (enclosingKind == Type::Kind::INTERFACE) {
        diag.error(fd) << "Intrinsic function cannot be defined within an interface.";
      }
    } else if (fd->ast()->body) {
      if (enclosingType && enclosingType->type()->kind == Type::Kind::INTERFACE) {
        diag.error(fd) << "A function within an interface definition cannot have a function body.";
      } else if (fd->isFinal()) {
        switch (enclosingKind) {
          case Type::Kind::STRUCT:
            diag.error(fd) << "Methods declared within a struct are always 'final'.";
            break;
          case Type::Kind::EXTENSION:
            diag.error(fd) << "Methods declared within a type extension are always 'final'.";
            break;
          default:
            break;
        }
      }
    } else if (enclosingKind != Type::Kind::TRAIT && enclosingKind != Type::Kind::INTERFACE) {
      diag.error(fd) << "Non-abstract function must have a function body.";
    }

    if (fd->isConstructor()) {
      if (fd->isMutableSelf()) {
        diag.error(fd) << "Constructors are always mutable-self methods.";
      } else if (!enclosingType) {
        diag.error(fd) << "Constructor function must be a member of a type.";
      } else if (fd->isSetter() || fd->isGetter()) {
        diag.error(fd) << "Constructor function can not also be a getter / setter.";
      } else if (enclosingKind == Type::Kind::EXTENSION) {
        diag.error(fd) << "A constructor may not be declared within a type extension.";
      }
      fd->setMutableSelf(true);
    } else if (fd->isGetter() && !fd->ast()->returnType) {
      diag.error(fd) << "Getter method must declare a return type.";
    } else if (fd->isSetter()) {
      if (fd->isMutableSelf()) {
        diag.error(fd) << "Setters are always mutable-self methods.";
      }
      fd->setMutableSelf(true);
      if (fd->ast()->returnType) {
        diag.error(fd) << "Setter method may not declare a return type.";
      } else if (fd->params().empty()) {
        diag.error(fd) << "Setter method must declare at least one method parameter.";
      }
    }

    visitTypeParams(scope, fd);

    TypeParamScope tpScope(scope, fd); // Scope used in resolving param types only.
    Type* returnType = nullptr;
    if (fd->ast()->returnType) {
      returnType = resolveType(&tpScope, fd->ast()->returnType, true);
    } else if (!fd->ast()->body) {
      diag.error(fd) << "No function body, function return type cannot be inferred.";
    }

    // if defns.isGlobalDefn(func):
    //   elif func.hasSelfType():
    //     self.errorAt(func,
    //         "Global function '(0)' cannot declare a 'self' parameter.", func.getName())
    //     func.clearSelfType()
    // elif defns.isStaticDefn(func):
    //   if func.hasSelfType():
    //     self.errorAt(func,
    //         "Static function '(0)' cannot declare a 'self' parameter.", func.getName())
    //     funcType.clearSelfType()

    // if func.getName() == 'new' and not func.isStatic():
    //   assert func.hasSelfType()

    if (!fd->selfType() && enclosingType) {
      assert(enclosingType->type());
      const Type* selfType = enclosingType->type();
      if (!fd->isMutableSelf()) {
        selfType = _cu.types().createModifiedType(selfType, ModifiedType::IMMUTABLE);
      }
      fd->setSelfType(selfType);
    }

    SmallVector<const Type*, 8> paramTypes;
    for (auto param : fd->params()) {
      visitAttributes(&tpScope, param, param->ast());
      if (param->ast()->type) {
        param->setType(resolveType(&tpScope, param->ast()->type, true));
        if (param->type()->kind == Type::Kind::TRAIT) {
          diag.error(param) << "Parameter type cannot be a trait.";
        }
        paramTypes.push_back(param->type());
      } else {
        diag.error(param) << "Parameter type is required";
      }
      if (param->ast()->init) {
        param->setInit(visitExpr(&tpScope, param->ast()->init));
      }
    }

    if (fd->ast()->body) {
      FunctionScope fnScope(scope, fd);
      auto saveFunction = _func;
      _func = fd;
      fd->setBody(visitExpr(&fnScope, fd->ast()->body));
      _func = saveFunction;
    }

    // If we know the return type now, then create a function type, otherwise we'll do it
    // in the type resolution pass.
    if (returnType) {
      fd->setType(_cu.types().createFunctionType(returnType, paramTypes, fd->isVariadic()));
    }
  }

  void NameResolutionPass::visitValueDefn(LookupScope* scope, ValueDefn* vd) {
    auto enclosingType = dyn_cast_or_null<TypeDefn>(vd->definedIn());
    auto enclosingKind = enclosingType ? enclosingType->type()->kind : Type::Kind::VOID;

    if (enclosingKind == Type::Kind::EXTENSION) {
      diag.error(vd) << "Type extensions cannot declare member fields.";
    }

    visitAttributes(scope, vd, vd->ast());
    if (vd->ast()->type) {
      vd->setType(resolveType(scope, vd->ast()->type, true));
      if (vd->type()->kind == Type::Kind::TRAIT) {
        diag.error(vd) << "Variable type cannot be a trait.";
      }
    }
    if (vd->ast()->init) {
      vd->setInit(visitExpr(scope, vd->ast()->init));
    }

    if (!vd->type() && !vd->init()) {
      diag.error(vd) << "No initializer, type cannot be inferred.";
    } else if (vd->isConstant() && vd->isStatic() && !vd->init()) {
      diag.error(vd) << "Static constant must be initialized to a value.";
    }

    if (!vd->isStatic()) {
      vd->setFieldIndex(_numInstanceVars++);
    }
  }

  void NameResolutionPass::visitAttributes(LookupScope* scope, Defn* defn, const ast::Defn* ast) {
    for (auto attr : ast->attributes) {
      if (attr->kind == ast::Node::Kind::BUILTIN_ATTRIBUTE) {
        auto ba = static_cast<const ast::BuiltinAttribute*>(attr);
        if (ba->attribute == ast::BuiltinAttribute::INTRINSIC) {
          if (!intrinsic::IntrinsicDefns::get()->registerExternal(defn)) {
            diag.error(defn->location()) << "Unknown intrinsic definition.";
          }
        } else {
          assert(false && "Implement");
        }
      } else {
        auto expr = visitExpr(scope, attr);
        if (!Expr::isError(expr)) {
          defn->attributes().push_back(expr);
        }
      }
    }
  }

  void NameResolutionPass::visitTypeParams(LookupScope* scope, GenericDefn* defn) {
    for (auto param : defn->typeParams()) {
      llvm::SmallVector<Type*, 8> subtypes;
      llvm::SmallVector<Type*, 8> supertypes;
      for (auto constraint : param->ast()->constraints) {
        if (constraint->kind == ast::Node::Kind::SUPERTYPE_CONSTRAINT) {
          auto type = resolveType(scope, static_cast<const ast::UnaryOp*>(constraint)->arg);
          if (!Type::isError(type)) {
            supertypes.push_back(type);
          }
        } else {
          auto type = resolveType(scope, constraint);
          if (!Type::isError(type)) {
            subtypes.push_back(type);
          }
        }
      }
      param->setSubtypeConstraints(_alloc->copyOf(subtypes));
      // param->setSupertypeConstraints(_alloc->copyOf(supertypes));
    }
  }

  Expr* NameResolutionPass::visitExpr(LookupScope* scope, const ast::Node* node) {
    switch (node->kind) {
      // case ast::Node::Kind::SELF: {
      //   break;
      // }
      // SUPER,

      case ast::Node::Kind::IDENT: {
        MemberLookupResult result;
        if (!resolveDefnName(scope, node, result)) {
          return &Expr::ERROR;
        }

        return createNameRef(*_alloc, node->location, result,
            cast_or_null<Defn>(scope->subject()),
            nullptr, false, false);
      }

      case ast::Node::Kind::SUPER: {
        return new (*_alloc) SuperExpr(node->location);
      }

      case ast::Node::Kind::SELF: {
        return new (*_alloc) SelfExpr(node->location);
      }

      case ast::Node::Kind::MEMBER: {
        auto memberRef = static_cast<const ast::MemberRef*>(node);
        auto stem = visitExpr(scope, memberRef->base);
        return new (*_alloc) MemberNameRef(
            Expr::Kind::MEMBER_NAME_REF, node->location, _alloc->copyOf(memberRef->name), stem);
      }
      // SELF_NAME_REF,
      // BUILTIN_ATTRIBUTE,
      // BUILTIN_TYPE,
      // KEYWORD_ARG,

      case ast::Node::Kind::BOOLEAN_TRUE:
        return new (*_alloc) BooleanLiteral(node->location, true);

      case ast::Node::Kind::BOOLEAN_FALSE:
        return new (*_alloc) BooleanLiteral(node->location, false);

      case ast::Node::Kind::CHAR_LITERAL: {
        auto literal = static_cast<const ast::Literal*>(node);
        std::wstring wideValue;
        if (!ConvertUTF8toWide(literal->value, wideValue)) {
          diag.error(node) << "Invalid character literal";
          return &Expr::ERROR;
        } else if (wideValue.size() < 1) {
          diag.error(node) << "Empty character literal";
          return &Expr::ERROR;
        } else if (wideValue.size() > 1) {
          diag.error(node) << "Character literal can only hold a single character";
          return &Expr::ERROR;
        } else {
          llvm::APInt wcharIntVal(32, wideValue[0], false);
          return new (*_alloc) IntegerLiteral(
            node->location,
            _alloc->copyOf(wcharIntVal),
            &IntegerType::CHAR);
        }
      }

      case ast::Node::Kind::INTEGER_LITERAL: {
        auto literal = static_cast<const ast::Literal*>(node);
        bool isUnsigned = false;
        for (auto suffixChar : literal->suffix) {
          if (suffixChar == 'u' || suffixChar == 'U') {
            isUnsigned = true;
          } else {
            diag.error(node) << "Invalid integer suffix: '" << literal->suffix << "'";
            break;
          }
        }

        StringRef value = literal->value;
        uint8_t radix = 10;
        if (value.startswith("0x") || value.startswith("0X")) {
          radix = 16;
          value = value.substr(2);
        }

        // Integer literals default to 64 bits wide.
        APInt intVal(64, value, radix);
        return new (*_alloc) IntegerLiteral(
          node->location,
          _alloc->copyOf(intVal),
          _cu.types().createIntegerType(intVal, isUnsigned));
      }

      case ast::Node::Kind::FLOAT_LITERAL: {
        auto literal = static_cast<const ast::Literal*>(node);
        bool isSingle = false;
        for (auto suffixChar : literal->suffix) {
          if (suffixChar == 'f' || suffixChar == 'F') {
            isSingle = true;
          } else {
            diag.error(node) << "Invalid float suffix: '" << literal->suffix << "'";
            break;
          }
        }

        APFloat value(
            isSingle ? APFloat::IEEEsingle() : APFloat::IEEEdouble(),
            literal->value);
        return new (*_alloc) FloatLiteral(node->location, value, false,
            isSingle ? &FloatType::F32 : &FloatType::F64);
      }

      case ast::Node::Kind::STRING_LITERAL: {
        auto literal = static_cast<const ast::Literal*>(node);
        return new (*_alloc) StringLiteral(node->location, _alloc->copyOf(literal->value));
      }

      case ast::Node::Kind::LOGICAL_NOT: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto arg = visitExpr(scope, op->arg);
        return new (*_alloc) UnaryOp(Expr::Kind::NOT, arg->location, arg);
      }

      case ast::Node::Kind::NEGATE: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto arg = visitExpr(scope, op->arg);
        return new (*_alloc) UnaryOp(Expr::Kind::NEGATE, arg->location, arg);
      }

      case ast::Node::Kind::COMPLEMENT: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto arg = visitExpr(scope, op->arg);
        return new (*_alloc) UnaryOp(Expr::Kind::COMPLEMENT, arg->location, arg);
      }

      // PRE_INC,
      // POST_INC,
      // PRE_DEC,
      // POST_DEC,
      // PROVISIONAL_CONST,
      // OPTIONAL,

      case ast::Node::Kind::ADD: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        auto expr = new (*_alloc) BinaryOp(
            Expr::Kind::ADD, lhs->location | rhs->location, lhs, rhs);
        return expr;
      }

      case ast::Node::Kind::SUB: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        auto expr = new (*_alloc) BinaryOp(
            Expr::Kind::SUBTRACT, lhs->location | rhs->location, lhs, rhs);
        return expr;
      }

      case ast::Node::Kind::MUL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        auto expr = new (*_alloc) BinaryOp(
            Expr::Kind::MULTIPLY, lhs->location | rhs->location, lhs, rhs);
        return expr;
      }

      case ast::Node::Kind::DIV: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        auto expr = new (*_alloc) BinaryOp(
            Expr::Kind::DIVIDE, lhs->location | rhs->location, lhs, rhs);
        return expr;
      }

      case ast::Node::Kind::MOD: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        auto expr = new (*_alloc) BinaryOp(
            Expr::Kind::REMAINDER, lhs->location | rhs->location, lhs, rhs);
        return expr;
      }

      case ast::Node::Kind::BIT_AND: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::BIT_AND, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::BIT_OR: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::BIT_OR, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::BIT_XOR: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::BIT_XOR, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::RSHIFT: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::RSHIFT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LSHIFT: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::LSHIFT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::EQ, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::NOT_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::NE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::GREATER_THAN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::GT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::GREATER_THAN_OR_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::GE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LESS_THAN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::LT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LESS_THAN_OR_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::LE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::REF_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::REF_EQ, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::REF_NOT_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::REF_NE, lhs->location | rhs->location, lhs, rhs);
      }

      // IS_SUB_TYPE,
      // IS_SUPER_TYPE,

      case ast::Node::Kind::ASSIGN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[1]);
        return new (*_alloc) BinaryOp(Expr::Kind::ASSIGN, lhs->location | rhs->location, lhs, rhs);
      }

      // ASSIGN_ADD,
      // ASSIGN_SUB,
      // ASSIGN_MUL,
      // ASSIGN_DIV,
      // ASSIGN_MOD,
      // ASSIGN_BIT_AND,
      // ASSIGN_BIT_OR,
      // ASSIGN_BIT_XOR,
      // ASSIGN_RSHIFT,
      // ASSIGN_LSHIFT,
      // LOGICAL_AND,
      // LOGICAL_OR,
      // RANGE,
      // AS_TYPE,
      // IS,
      // IS_NOT,
      // IN,
      // NOT_IN,
      // RETURNS,
      // LAMBDA,
      // EXPR_TYPE,

      case ast::Node::Kind::RETURN: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto returnVal = op->arg ? visitExpr(scope, op->arg) : nullptr;
        return new (*_alloc) UnaryOp(Expr::Kind::RETURN, node->location, returnVal);
      }

      case ast::Node::Kind::UNSAFE: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto arg = visitExpr(scope, op->arg);
        return new (*_alloc) UnaryOp(Expr::Kind::UNSAFE, node->location, arg);
      }

      case ast::Node::Kind::THROW: {
        auto op = static_cast<const ast::UnaryOp*>(node);
        auto returnVal = op->arg ? visitExpr(scope, op->arg) : nullptr;
        return new (*_alloc) UnaryOp(Expr::Kind::THROW, node->location, returnVal);
      }

      case ast::Node::Kind::CALL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto fn = resolveFunctionName(scope, op->op);
        if (Expr::isError(fn)) {
          return fn;
        }

        llvm::SmallVector<Expr*, 8> args;
        for (auto arg : op->operands) {
          auto argExpr = visitExpr(scope, arg);
          if (!Expr::isError(argExpr)) {
            args.push_back(argExpr);
          }
        }

        return new (*_alloc) ApplyFnOp(Expr::Kind::CALL, op->location, fn, _alloc->copyOf(args));
      }

      case ast::Node::Kind::SPECIALIZE: {
        return visitSpecialize(scope, static_cast<const ast::Oper*>(node));
      }

      // FLUENT_MEMBER,
      // ARRAY_LITERAL,
      // LIST_LITERAL,
      // SET_LITERAL,
      // LIST,       // List of opions for switch cases, catch blocks, etc.

      case ast::Node::Kind::BLOCK: {
        auto block = static_cast<const ast::Block*>(node);
        LocalScope localScope(scope);
        llvm::SmallVector<Expr*, 8> stmts;
        for (auto st : block->stmts) {
          auto stExpr = visitExpr(&localScope, st);
          if (stExpr) {
            stmts.push_back(stExpr);
          }
        }

        Expr* result = nullptr;
        if (block->result) {
          result = visitExpr(&localScope, block->result);
        }

        return new (*_alloc) BlockStmt(node->location, _alloc->copyOf(stmts), result);
      }

      case ast::Node::Kind::LOCAL_CONST:
      case ast::Node::Kind::LOCAL_LET: {
        auto decl = static_cast<const ast::ValueDefn*>(node);
        ValueDefn* defn = new (*_alloc) ValueDefn(
          Defn::Kind::VAR_DEF,
          node->location,
          _alloc->copyOf(decl->name),
          _func
        );
        defn->setConstant(node->kind == ast::Node::Kind::LOCAL_CONST);
        if (decl->type) {
          defn->setType(resolveType(scope, decl->type, true));
        }
        if (decl->init) {
          defn->setInit(visitExpr(scope, decl->init));
        }

        if (!scope->addMember(defn)) {
          diag.error(node) << "Invalid scope for local definition.";
        }

        auto index = _func ? _func->localDefns().size() : 0;
        defn->setFieldIndex(index);
        defn->setLocal(true);
        auto st = new (*_alloc) LocalVarStmt(node->location, defn, index);
        if (_func) {
          _func->localDefns().push_back(defn);
        }
        return st;
      }

      case ast::Node::Kind::IF: {
        auto stmt = static_cast<const ast::Control*>(node);
        auto test = visitExpr(scope, stmt->test);
        auto thenSt = visitExpr(scope, stmt->outcomes[0]);
        auto elseSt = stmt->outcomes.size() > 1 ? visitExpr(scope, stmt->outcomes[1]) : nullptr;
        return new (*_alloc) IfStmt(node->location, test, thenSt, elseSt);
      }

      case ast::Node::Kind::WHILE: {
        auto stmt = static_cast<const ast::Control*>(node);
        auto test = visitExpr(scope, stmt->test);
        auto body = visitExpr(scope, stmt->outcomes[0]);
        return new (*_alloc) WhileStmt(node->location, test, body);
      }

      case ast::Node::Kind::LOOP: {
        auto stmt = static_cast<const ast::Control*>(node);
        auto test = new (*_alloc) BooleanLiteral(node->location, true);
        auto body = visitExpr(scope, stmt->outcomes[0]);
        return new (*_alloc) WhileStmt(node->location, test, body);
      }

      // /* Misc statements */
      // FINALLY,    // finally block for try

      // FOR,        // for (vars, init, test, step, body)
      // FOR_IN,     // for in (vars, iter, body)
      // TRY,        // try (test, body, cases...)
      // CATCH,      // catch (except-list, body)
      // SWITCH,     // switch (test, cases...)
      // CASE,       // switch case (values | [values...]), body
      // MATCH,      // match (test, cases...)
      // PATTERN,    // match pattern (pattern, body)

      // /* Type operators */
      // MODIFIED,
      // FUNCTION_TYPE,

      // /* Other statements */
      // BREAK,
      // CONTINUE,

      default:
        diag.error(node) << "Invalid expression type: " << ast::Node::KindName(node->kind);
        assert(false && "Invalid node kind");
    }
  }

  Expr* NameResolutionPass::visitSpecialize(LookupScope* scope, const ast::Oper* node) {
    auto base = visitExpr(scope, node->op);
    if (Expr::isError(base)) {
      return &Expr::ERROR;
    }
    if (base->kind != Expr::Kind::FUNCTION_REF_OVERLOAD &&
        base->kind != Expr::Kind::TYPE_REF_OVERLOAD) {
      diag.error(node) << "Expression cannot be specialized";
      return &Expr::ERROR;
    }

    llvm::SmallVector<Type*, 8> args;
    if (node->operands.empty()) {
      diag.error(node) << "Missing type arguments";
      return &Expr::ERROR;
    }

    for (auto arg : node->operands) {
      auto argType = resolveType(scope, arg);
      if (Type::isError(argType)) {
        return &Expr::ERROR;
      }
      args.push_back(argType);
    }

    auto argArray = _alloc->copyOf(args);
    auto baseRef = static_cast<MemberListExpr*>(base);
    llvm::SmallVector<MemberAndStem, 8> specMembers;
    specMembers.resize(baseRef->members.size());
    std::transform(baseRef->members.begin(), baseRef->members.end(), specMembers.begin(),
        [argArray, this](auto& m) {
          auto generic = cast<GenericDefn>(m.member);
          MemberAndStem result = {
            new (*_alloc) SpecializedDefn(generic, argArray, generic->typeParams()),
            m.stem
          };
          return result;
        });
    return new (*_alloc) MemberListExpr(
        base->kind, base->location, baseRef->name, _alloc->copyOf(specMembers));
  }

  Type* NameResolutionPass::resolveType(
      LookupScope* scope, const ast::Node* node, bool implicitSpecialization) {
    switch (node->kind) {
      // What these all have in common is that they resolve to a definition, possibly
      // specialized, and then determine if that definition is a type.
      case ast::Node::Kind::IDENT:
      case ast::Node::Kind::MEMBER:
      case ast::Node::Kind::SPECIALIZE: {
        MemberLookupResult lookupResult;
        if (!resolveDefnName(scope, node, lookupResult)) {
          return &Type::ERROR;
        }

        llvm::SmallVector<Type*, 8> types;
        for (auto m : lookupResult) {
          if (m.member->kind == Member::Kind::TYPE) {
            // If it's a bare reference to a generic type, and the type is the same as
            // the enclosing type definition, then automatically specialize it.
            if (implicitSpecialization &&
                m.member == _typeDefn &&
                _typeDefn->allTypeParams().size() > 0) {
              SmallVector<const Type*, 4> typeArgs;
              for (auto tp : _typeDefn->allTypeParams()) {
                typeArgs.push_back(tp->typeVar());
              }
              auto sp = _cu.spec().specialize(_typeDefn, typeArgs);
              types.push_back(sp->type());
            } else {
              types.push_back(static_cast<TypeDefn*>(m.member)->type());
            }
          } else if (m.member->kind == Member::Kind::TYPE_PARAM) {
            types.push_back(static_cast<TypeParameter*>(m.member)->typeVar());
          } else if (m.member->kind == Member::Kind::SPECIALIZED) {
            auto spec = static_cast<SpecializedDefn*>(m.member);
            if (spec->generic()->kind == Member::Kind::TYPE) {
              return simplifyTypeSpecialization(spec);
            } else {
              diag.error(node) << "Expecting a type name.";
              return &Type::ERROR;
            }
          } else if (m.member->kind == Member::Kind::ENUM_VAL) {
            // Singleton type
            assert(false && "Implement");
          } else {
            diag.error(node) << "Expecting a type name.";
            return &Type::ERROR;
          }
        }

        NameLookupResult privateMembers;
        NameLookupResult protectedMembers;
        auto subject = scope->subject();
        for (auto member : lookupResult) {
          auto m = unwrapSpecialization(member.member);
          if (!isVisibleMember(subject, m)) {
            if (m->visibility() == PRIVATE) {
              privateMembers.push_back(m);
            } else if (m->visibility() == PROTECTED) {
              protectedMembers.push_back(m);
            }
          }
        }

        if (privateMembers.size() > 0) {
          auto p = unwrapSpecialization(privateMembers[0]);
          diag.error(node) << "Cannot access private member '" << p->name() << "'.";
          diag.info(p->location()) << "Defined here.";
        } else if (protectedMembers.size() > 0) {
          auto p = unwrapSpecialization(protectedMembers[0]);
          diag.error(node) << "Cannot access protected member '" << p->name() << "'.";
          diag.info(p->location()) << "Defined here.";
        }

        assert(!types.empty());
        if (types.size() == 1) {
          return types[0];
        }
        diag.error() << "Type is ambigous, " << types.size() << " possible matches:";
        for (auto& m : lookupResult) {
          diag.info() << m.member;
        }
        // for (auto t : types) {
        //   diag.info() << t;
        // }
        assert(false && "Implement ambiguous types");
        break;
      }

      case ast::Node::Kind::UNION_TYPE: {
        llvm::SmallVector<Type*, 8> unionMembers;
        std::function<void(const ast::Node*)> flatUnion;
        flatUnion = [this, scope, &unionMembers, &flatUnion](const ast::Node* in) -> void {
          if (in->kind == ast::Node::Kind::UNION_TYPE) {
            auto unionOp = static_cast<const ast::Oper*>(in);
            for (auto oper : unionOp->operands) {
              flatUnion(oper);
            }
          } else {
            auto t = resolveType(scope, in, true);
            if (t) {
              // TODO: check if it's a union here as well.
              unionMembers.push_back(t);
            }
          }
        };
        flatUnion(node);
        // TODO: Do we need to ensure uniqueness here?
        return _cu.types().createUnionType(unionMembers);
      }

      case ast::Node::Kind::TUPLE_TYPE: {
        llvm::SmallVector<Type*, 8> tupleMembers;
        auto tupleOp = static_cast<const ast::Oper*>(node);
        for (auto oper : tupleOp->operands) {
          tupleMembers.push_back(resolveType(scope, oper, true));
        }
        return _cu.types().createTupleType(tupleMembers);
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

      case ast::Node::Kind::INTEGER_LITERAL:
      case ast::Node::Kind::CHAR_LITERAL:
      case ast::Node::Kind::STRING_LITERAL: {
        auto expr = visitExpr(scope, node);
        if (Expr::isError(expr)) {
          return &Type::ERROR;
        }
        return _cu.types().createSingletonType(expr);
      }

      case ast::Node::Kind::ADD:
      case ast::Node::Kind::SUB:
      case ast::Node::Kind::DIV:
      case ast::Node::Kind::MUL:
      case ast::Node::Kind::MOD:
      case ast::Node::Kind::LSHIFT:
      case ast::Node::Kind::RSHIFT: {
        // Singleton type
        return &Type::ERROR;
        break;
      }

      default:
        diag.error(node) << "Invalid node kind: " << ast::Node::KindName(node->kind);
        assert(false && "Invalid AST node for type");
        return &Type::ERROR;
    }
  }

  Expr* NameResolutionPass::resolveFunctionName(LookupScope* scope, const ast::Node* node) {
    // Function names are allowed to return an empty result because it can be filled in later
    // via ADL.
    if (node->kind == ast::Node::Kind::IDENT) {
      auto ident = static_cast<const ast::Ident*>(node);
      MemberLookupResult result;
      if (!resolveDefnName(scope, node, result, true)) {
        return &Expr::ERROR;
      }

      if (result.size() > 0) {
        return createNameRef(*_alloc, node->location, result,
            cast_or_null<Defn>(scope->subject()),
            nullptr, false, true);
      } else {
        auto mref = new (*_alloc) MemberListExpr(
            Expr::Kind::FUNCTION_REF_OVERLOAD, ident->location, ident->name,
            llvm::ArrayRef<MemberAndStem>());
        mref->useADL = true;
        return mref;
      }
    } else {
      return visitExpr(scope, node);
    }
  }

  Expr* NameResolutionPass::resolveOperatorName(
      const Location& loc, LookupScope* scope, const StringRef& name) {
    MemberLookupResult result;
    scope->lookup(name, result);
    if (result.size() > 0) {
      return createNameRef(*_alloc, loc, result,
          cast_or_null<Defn>(scope->subject()),
          nullptr, false, true);
    } else {
      auto mref = new (*_alloc) MemberListExpr(
          Expr::Kind::FUNCTION_REF_OVERLOAD, loc, name,
          llvm::ArrayRef<MemberAndStem>());
      mref->useADL = true;
      return mref;
    }
  }

  // This handles compound names such as A, A.B.C, A[X].B[Y], but not (expression).B.
  // The result is an array of possibly overloaded definitions.
  bool NameResolutionPass::resolveDefnName(
    LookupScope* scope, const ast::Node* node, MemberLookupResultRef& result, bool allowEmpty) {
    switch (node->kind) {
      case ast::Node::Kind::IDENT: {
        auto ident = static_cast<const ast::Ident*>(node);
        scope->lookup(ident->name, result);
        if (result.empty()) {
          if (allowEmpty) {
            return true;
          }
          diag.error(node) << "Name not found: " << ident->name;
          ClosestName closest(ident->name);
          scope->forEach(std::ref(closest));
          if (!closest.bestMatch.empty()) {
            diag.info() << "Did you mean '" << closest.bestMatch << "'?";
          }
          return false;
        }
        return true;
      }

      case ast::Node::Kind::MEMBER: {
        auto memberRef = static_cast<const ast::MemberRef*>(node);
        MemberLookupResult baseResult;
        if (!resolveDefnName(scope, memberRef->base, baseResult)) {
          return false;
        }
        // TODO: We should ensure baseResult is a type

        for (auto baseDefn : baseResult) {
          resolveMemberName(node->location, baseDefn.member, memberRef->name, result);
        }

        if (result.empty()) {
          diag.error(node) << "Name not found: " << memberRef->name;
          return false;
        }

        return true;
      }

      case ast::Node::Kind::SPECIALIZE: {
        auto spec = static_cast<const ast::Oper*>(node);
        // TODO: Shouldn't this be resolveDefnName?
        auto base = resolveType(scope, spec->op);
        if (Type::isError(base)) {
          return false;
        }
        llvm::SmallVector<Type*, 8> typeArgs;
        for (auto param : spec->operands) {
          auto paramType = resolveType(scope, param, true);
          if (Type::isError(paramType)) {
            return false;
          }
          typeArgs.push_back(paramType);
        }

        // Valid base types.
        if (auto udt = dyn_cast<UserDefinedType>(base)) {
          // Class, struct, etc. In other words, a potential generic type.
          if (auto gd = dyn_cast<GenericDefn>(udt->defn())) {
            if (typeArgs.size() != gd->typeParams().size()) {
              if (gd->typeParams().empty()) {
                diag.error(node) << "Definition '" << gd->name() << "' is not generic.";

              } else {
                diag.error(node) << "Generic definition '" << gd->name() << "' requires "
                    << gd->typeParams().size() << " type arguments, "
                    << typeArgs.size() << " were provided.";
              }
              return false;
            }

            auto sd = _cu.spec().specialize(gd, typeArgs);
            result.push_back({ sd, nullptr });
            return true;
          } else {
            diag.error(node) << "Can't specialize a non-generic type.";
            return false;
          }
        } else if (base->kind == Type::Kind::SPECIALIZED) {
          diag.error(node) << "Can't specialize already-specialized type.";
          return false;
        } else {
          diag.error(node) << "Can't specialize non-generic type.";
          return false;
        }
      }

      case ast::Node::Kind::BUILTIN_TYPE: {
        // We can get here via 'enum extends' - enums can have integer bases.
        auto type = resolveType(scope, node);
        result.push_back({ cast<IntegerType>(type)->defn(), nullptr });
        return true;
      }

      default:
        diag.error(node) << "Invalid node kind for resolveDefnName: "
            << ast::Node::KindName(node->kind);
        assert(false && "Bad node kind");
    }
    return false;
  }

  bool NameResolutionPass::resolveMemberName(
      const Location& loc,
      Member* scope,
      const StringRef& name,
      MemberLookupResultRef& result) {
    // Note: It is not an error for this function to return an empty result. During overload
    // resolution, some overloads may have a defined member with the given name and some may not.
    if (auto typeDef = dyn_cast<TypeDefn>(scope)) {
      auto resultSize = result.size();
      typeDef->memberScope()->lookup(name, result, nullptr);
      if (result.size() > resultSize) {
        return true;
      }
      if (typeDef->type()->kind == Type::Kind::CLASS ||
          typeDef->type()->kind == Type::Kind::INTERFACE ||
          typeDef->type()->kind == Type::Kind::STRUCT ||
          typeDef->type()->kind == Type::Kind::TRAIT) {
        eagerResolveBaseTypes(typeDef);
        for (auto extBase : typeDef->extends()) {
          resolveMemberName(loc, extBase, name, result);
        }
        if (result.size() > resultSize) {
          return true;
        }
      } else if (typeDef->type()->kind == Type::Kind::ALIAS) {
        auto target = typeDef->aliasTarget();
        if (auto udt = dyn_cast<UserDefinedType>(target)) {
          return resolveMemberName(loc, udt->defn(), name, result);
        } else if (auto sp = dyn_cast<SpecializedType>(target)) {
          return resolveMemberName(loc, const_cast<SpecializedDefn*>(sp->spec), name, result);
        }
        return false;
      }
      return false;
    } else if (auto typeParam = dyn_cast<TypeParameter>(scope)) {
      // To do this lookup we're going to have to analyze the type constraints. Oh boy!
      assert(false && "Implement");
    } else if (auto baseTypeDef = dyn_cast<ValueDefn>(scope)) {
      // return new (*_alloc) Mem
      assert(false && "Implement");
    } else if (auto specDefn = dyn_cast<SpecializedDefn>(scope)) {
      if (specDefn->generic()->kind == Member::Kind::SPECIALIZED) {
        diag.error(loc) << "Generic definition is already specialized.";
        return false;
      } else if (specDefn->generic()->kind == Member::Kind::FUNCTION) {
        diag.error(loc) << "Can't access a member of a function.";
        return false;
      } else if (!isa<GenericDefn>(specDefn->generic())) {
        diag.error(loc) << "Definition is not generic.";
        return false;
      }

      // Recursively call this function on the non-specialized generic base definition.
      MemberLookupResult specLookup;
      resolveMemberName(loc, specDefn->generic(), name, specLookup);
      for (auto member : specLookup) {
        // TODO: If member.member is already specialized, then compose type arguments.
        auto specMember = new (*_alloc) SpecializedDefn(
            cast<Defn>(member.member), specDefn->typeArgs(), specDefn->typeParams());
        if (isa<TypeDefn>(member.member)) {
          specMember->setType(new (*_alloc) SpecializedType(specMember));
        }
        result.push_back({ specMember, nullptr });
      }
      return true;
    } else {
      assert(false && "Bad defn kind");
    }
  }

  // Used when we need to lookup an inherited member before that class has been processed.
  void NameResolutionPass::eagerResolveBaseTypes(TypeDefn* td) {
    if (td->baseTypesResolved()) {
      return;
    }

    llvm::SmallVector<std::unique_ptr<LookupScope>, 8> enclosingScopes;
    std::function<void (Member* m)> findScopes;

    // Temporarily create a chain of enclosing lookup scopes.
    findScopes = [&enclosingScopes, &findScopes, this](Member* m) {
      if (m) {
        // Do the outermost scopes first so they will be in order.
        findScopes(m->definedIn());
        if (auto mod = dyn_cast<Module>(m)) {
          enclosingScopes.push_back(
              std::make_unique<ModuleScope>(nullptr, mod));
        } else if (auto typeDefn = dyn_cast<TypeDefn>(m)) {
          enclosingScopes.push_back(
              std::make_unique<TypeDefnScope>(enclosingScopes.back().get(), typeDefn, _cu.spec()));
        }
      }
    };
    findScopes(td->definedIn());
    resolveBaseTypes(enclosingScopes.back().get(), td);
  }

  void NameResolutionPass::resolveBaseTypes(LookupScope* scope, TypeDefn* td) {
    if (td->baseTypesResolved()) {
      return;
    }
    td->setBaseTypesResolved(true);

    auto astNode = static_cast<const ast::TypeDefn*>(td->ast());
    for (auto astBase : astNode->extends) {
      MemberLookupResult result;
      if (resolveDefnName(scope, astBase, result)) {
        if (result.size() > 1) {
          diag.error(astBase) << "Ambiguous base type.";
        }
        auto& base = result[0];
        auto baseKind = typeKindOf(base.member);
        auto baseDefn = cast<TypeDefn>(unwrapSpecialization(base.member));
        switch (td->type()->kind) {
          case Type::Kind::CLASS:
            if (baseKind != Type::Kind::CLASS) {
              diag.error(astBase->location) << "A class can only extend another class.";
            } else if (!td->extends().empty()) {
              diag.error(astBase->location) <<
                  "Multiple implementation inheritance is not allowed.";
            } else if (baseDefn->intrinsic() == IntrinsicType::FLEXALLOC_CLASS) {
              if (!td->isFinal()) {
                diag.error(astBase->location) <<
                    "A class extending FlexAlloc must be declared as final.";
              }
              td->setFlex(true);
            }
            break;
          case Type::Kind::STRUCT:
            if (baseKind != Type::Kind::STRUCT) {
              diag.error(astBase->location) << "A struct can only extend another struct.";
            } else if (!td->extends().empty()) {
              diag.error(astBase->location) <<
                  "Multiple implementation inheritance is not allowed.";
            }
            break;
          case Type::Kind::INTERFACE:
            if (baseKind != Type::Kind::INTERFACE) {
              diag.error(astBase->location) << "An interface can only extend another interface.";
            }
            break;
          case Type::Kind::TRAIT:
            if (baseKind != Type::Kind::TRAIT) {
              diag.error(astBase->location) << "A trait can only extend another trait.";
            }
            break;
          case Type::Kind::EXTENSION:
            diag.error(astBase->location) << "Extensions cannot be extended.";
            break;
          case Type::Kind::ENUM:
            if (baseKind != Type::Kind::INTEGER) {
              diag.error(astBase->location) << "Enumations can only extend integer types.";
            } else if (!td->extends().empty()) {
              diag.error(astBase->location) <<
                  "Multiple implementation inheritance is not allowed.";
            }
            break;
          default:
            assert(false && "Type cannot extend");
        }
        td->extends().push_back(base.member);
      }
    }

    for (auto astBase : astNode->implements) {
      MemberLookupResult result;
      if (resolveDefnName(scope, astBase, result)) {
        if (result.size() > 1) {
          diag.error(astBase) << "Ambiguous base type.";
        }
        auto& base = result[0];
        auto baseKind = typeKindOf(base.member);
        switch (td->type()->kind) {
          case Type::Kind::CLASS:
            if (baseKind != Type::Kind::INTERFACE && baseKind != Type::Kind::TRAIT) {
              diag.error(astBase->location) << "A class can only implement an interface or trait.";
            }
            break;
          case Type::Kind::STRUCT:
            if (baseKind != Type::Kind::TRAIT) {
              diag.error(astBase->location) << "A struct can only implement a trait.";
            }
            break;
          case Type::Kind::INTERFACE:
            diag.error(astBase->location) <<
                "'implements' is not allowed for interfaces, use 'extends'.";
            break;
          case Type::Kind::TRAIT:
            diag.error(astBase->location) <<
                "'implements' is not allowed for traits, use 'extends'.";
            break;
          case Type::Kind::EXTENSION:
            // diag.error(astBase->location) << "Extensions cannot be extended.";
            break;
          case Type::Kind::ENUM:
            diag.error(astBase->location) << "Extensions cannot implement other types.";
            break;
          default:
            assert(false && "Type cannot implement");
        }
        td->implements().push_back(base.member);
      }
    }

    if (td->extends().empty()) {
      if (td->type()->kind == Type::Kind::CLASS) {
        // Classes inherit from 'Object' if no base class specified.
        td->extends().push_back(intrinsic::IntrinsicDefns::get()->objectClass.get());
      } else if (td->type()->kind == Type::Kind::ENUM) {
        // Enums inherit from 'u32' if no base class specified.
        td->extends().push_back(IntegerType::U32.defn());
      }
    }

    for (size_t i = 0; i < td->implements().size(); i += 1) {
      for (size_t j = i + 1; j < td->implements().size(); j += 1) {
        ArrayRef<const Type*> e0;
        ArrayRef<const Type*> e1;
        auto b0 = unwrapSpecialization(td->implements()[i], e0);
        auto b1 = unwrapSpecialization(td->implements()[j], e1);
        if (b0 == b1 && convert::isEqual(e0, e1)) {
          diag.error(td->location()) <<
            "Type implements the same interface more than once.";
        }
      }
    }
  }

  Type* NameResolutionPass::simplifyTypeSpecialization(SpecializedDefn* specDefn) {
    if (auto td = dyn_cast<TypeDefn>(specDefn->generic())) {
      if (auto pt = dyn_cast<PrimitiveType>(td->type())) {
        // Primitive types have no type params.
        return pt;
      } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
        // If it's not a generic type, then strip off all the specialization stuff.
        if (udt->defn()->allTypeParams().size() == 0) {
          return udt;
        }
      }
      // TODO: We could simplify unions and other derived types if feeling ambitious.
    }

    // Otherwise, just return the specialized type.
    return specDefn->type();
  }
}
