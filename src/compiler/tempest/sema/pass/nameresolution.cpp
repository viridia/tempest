#include "tempest/ast/defn.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/names/closestname.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;

  namespace {
    // Given a (possibly specialized) type definition, return what kind of type it is.
    Type::Kind typeKindOf(Member* m) {
      if (auto sp = dyn_cast<SpecializedDefn>(m)) {
        m = sp->generic();
      }

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
    // diag.info() << "Resolving imports: " << mod->name();
    _alloc = &mod->semaAlloc();
    resolveImports(mod);
    ModuleScope scope(nullptr, mod);
    visitList(&scope, mod->members());
    // createMembers(modAst->members, mod, mod->members(), mod->memberScope());
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
      resolveBaseTypes(scope, td);
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
    if (vd->ast()->type) {
      vd->setType(resolveType(scope, vd->ast()->type));
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

  Expr* NameResolutionPass::visitExpr(LookupScope* scope, const ast::Node* node) {
    switch (node->kind) {
      // case ast::Node::Kind::SELF: {
      //   break;
      // }
      // SUPER,

      // IDENT,
      // MEMBER,
      // SELF_NAME_REF,
      // BUILTIN_ATTRIBUTE,
      // BUILTIN_TYPE,
      // KEYWORD_ARG,

      // /* Literals */
      case ast::Node::Kind::BOOLEAN_TRUE: {
        return new (*_alloc) BooleanLiteral(node->location, true);
      }

      case ast::Node::Kind::BOOLEAN_FALSE: {
        return new (*_alloc) BooleanLiteral(node->location, false);
      }

      // CHAR_LITERAL,
      // INTEGER_LITERAL,
      // FLOAT_LITERAL,
      // STRING_LITERAL,

      // NEGATE,
      // COMPLEMENT,
      // LOGICAL_NOT,
      // PRE_INC,
      // POST_INC,
      // PRE_DEC,
      // POST_DEC,
      // STATIC,
      // CONST,
      // PROVISIONAL_CONST,
      // OPTIONAL,

      case ast::Node::Kind::ADD: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::ADD, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::SUB: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::SUBTRACT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::MUL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::MULTIPLY, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::DIV: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::DIVIDE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::MOD: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(
            Expr::Kind::REMAINDER, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::BIT_AND: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::BIT_AND, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::BIT_OR: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::BIT_OR, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::BIT_XOR: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::BIT_XOR, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::RSHIFT: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::RSHIFT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LSHIFT: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::LSHIFT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::EQ, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::NOT_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::NE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::GREATER_THAN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::GT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::GREATER_THAN_OR_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::GE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LESS_THAN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::LT, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::LESS_THAN_OR_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::LE, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::REF_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::REF_EQ, lhs->location | rhs->location, lhs, rhs);
      }

      case ast::Node::Kind::REF_NOT_EQUAL: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::REF_NE, lhs->location | rhs->location, lhs, rhs);
      }

      // IS_SUB_TYPE,
      // IS_SUPER_TYPE,

      case ast::Node::Kind::ASSIGN: {
        auto op = static_cast<const ast::Oper*>(node);
        auto lhs = visitExpr(scope, op->operands[0]);
        auto rhs = visitExpr(scope, op->operands[0]);
        return new (*_alloc) InfixOp(Expr::Kind::ASSIGN, lhs->location | rhs->location, lhs, rhs);
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
      // RETURN,
      // THROW,

      // /* N-ary operators */
      // TUPLE_TYPE,
      // UNION_TYPE,
      // ARRAY_TYPE,
      // SPECIALIZE,
      // CALL,
      // FLUENT_MEMBER,
      // ARRAY_LITERAL,
      // LIST_LITERAL,
      // SET_LITERAL,
      // CALL_REQUIRED,
      // CALL_REQUIRED_STATIC,
      // LIST,       // List of opions for switch cases, catch blocks, etc.

      // /* Misc statements */
      // BLOCK,      // A statement block
      // LOCAL_LET,  // A single variable definition (ident, type, init)
      // LOCAL_CONST,// A single variable definition (ident, type, init)
      // ELSE,       // default for match or switch
      // FINALLY,    // finally block for try

      // IF,         // if-statement (test, thenBlock, elseBlock)
      // WHILE,      // while-statement (test, body)
      // LOOP,       // loop (body)
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

      // /* Definitions */
      // /* TODO: Move this outside */
      // VISIBILITY,

      // DEFN,
      // TYPE_DEFN,
      // CLASS_DEFN,
      // STRUCT_DEFN,
      // INTERFACE_DEFN,
      // TRAIT_DEFN,
      // EXTEND_DEFN,
      // OBJECT_DEFN,
      // ENUM_DEFN,
      // MEMBER_VAR,
      // MEMBER_CONST,
      // // VAR_LIST,   // A list of variable definitions
      // ENUM_VALUE,
      // PARAMETER,
      // TYPE_PARAMETER,
      // FUNCTION,
      // DEFN_END,

      // MODULE,
      // IMPORT,
      // EXPORT,
      default:
        diag.error(node) << "Invalid expression type: " << ast::Node::KindName(node->kind);
        assert(false && "Invalid node kind");
    }
  }

  Type* NameResolutionPass::resolveType(LookupScope* scope, const ast::Node* node) {
    switch (node->kind) {
      // What these all have in common is that they resolve to a definition, possibly
      // specialized, and then determine if that definition is a type.
      case ast::Node::Kind::IDENT:
      case ast::Node::Kind::MEMBER:
      case ast::Node::Kind::SPECIALIZE: {
        NameLookupResult lookupResult;
        if (!resolveDefnName(scope, node, lookupResult)) {
          return &Type::ERROR;
        }

        // TODO: make sure that all of the lookups are compatible kinds.
        llvm::SmallVector<Type*, 8> types;
        for (auto member : lookupResult) {
          if (member->kind == Member::Kind::TYPE) {
            types.push_back(static_cast<TypeDefn*>(member)->type());
          } else if (member->kind == Member::Kind::TYPE_PARAM) {
            types.push_back(static_cast<TypeParameter*>(member)->typeVar());
          } else if (member->kind == Member::Kind::SPECIALIZED) {
            auto spec = static_cast<SpecializedDefn*>(member);
            if (spec->generic()->kind == Member::Kind::TYPE) {
              return simplifyTypeSpecialization(spec);
            } else {
              diag.error(node) << "Expecting a type name.";
              return &Type::ERROR;
            }
          } else if (member->kind == Member::Kind::ENUM_VAL) {
            // Singleton type
            assert(false && "Implement");
          } else {
            diag.error(node) << "Expecting a type name.";
            return &Type::ERROR;
          }
        }

        assert(!types.empty());
        if (types.size() == 1) {
          return types[0];
        }
        diag.error() << "Type is ambigous, " << types.size() << " possible matches:";
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
            auto t = resolveType(scope, in);
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
          tupleMembers.push_back(resolveType(scope, oper));
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
      case ast::Node::Kind::STRING_LITERAL:
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

  bool NameResolutionPass::resolveDefnName(
    LookupScope* scope, const ast::Node* node, NameLookupResultRef& result) {
    switch (node->kind) {
      case ast::Node::Kind::IDENT: {
        auto ident = static_cast<const ast::Ident*>(node);
        scope->lookup(ident->name, result);
        if (result.empty()) {
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
        NameLookupResult baseResult;
        if (!resolveDefnName(scope, memberRef->base, baseResult)) {
          return false;
        }

        for (auto baseDefn : baseResult) {
          resolveMemberName(node->location, baseDefn, memberRef->name, result);
        }

        if (result.empty()) {
          diag.error(node) << "Name not found: " << memberRef->name;
          return false;
        }

        // TODO: Ensure that all of the results are of a compatible kind
        // Or possibly do that earlier if we can? Also?
        return true;
      }

      case ast::Node::Kind::SPECIALIZE: {
        auto spec = static_cast<const ast::Oper*>(node);
        auto base = resolveType(scope, spec->op);
        if (Type::isError(base)) {
          return false;
        }
        llvm::SmallVector<Type*, 8> typeArgs;
        for (auto param : spec->operands) {
          auto paramType = resolveType(scope, param);
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
              diag.error(node) << "Generic definition requires " << gd->typeParams().size() <<
                  " type arguments, " << typeArgs.size() << " were provided.";
              return false;
            }

            auto sd = _cu.types().specialize(gd, typeArgs);
            result.push_back(sd);
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
      NameLookupResultRef& result) {
    // Note: It is not an error for this function to return an empty result. During overload
    // resolution, some overloads may have a defined member with the given name and some may not.
    if (auto typeDef = dyn_cast<TypeDefn>(scope)) {
      auto resultSize = result.size();
      typeDef->memberScope()->lookupName(name, result);
      if (result.size() > resultSize) {
        return true;
      }
      if (typeDef->type()->kind == Type::Kind::CLASS) {
        eagerResolveBaseTypes(typeDef);
        for (auto extBase : typeDef->extends()) {
          resolveMemberName(loc, extBase, name, result);
        }
        if (result.size() > resultSize) {
          return true;
        }
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
      NameLookupResult specLookup;
      resolveMemberName(loc, specDefn->generic(), name, specLookup);
      for (auto member : specLookup) {
        auto specMember = new (*_alloc) SpecializedDefn(
            member, specDefn->typeArgs(), specDefn->typeParams());
        if (isa<TypeDefn>(member)) {
          specMember->setType(new (*_alloc) SpecializedType(specMember));
        }
        result.push_back(specMember);
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
    findScopes = [&enclosingScopes, &findScopes](Member* m) {
      if (m) {
        // Do the outermost scopes first so they will be in order.
        findScopes(m->definedIn());
        if (auto mod = dyn_cast<Module>(m)) {
          enclosingScopes.push_back(
              std::make_unique<ModuleScope>(nullptr, mod));
        } else if (auto typeDefn = dyn_cast<TypeDefn>(m)) {
          enclosingScopes.push_back(
              std::make_unique<TypeDefnScope>(enclosingScopes.back().get(), typeDefn));
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
      NameLookupResult result;
      if (resolveDefnName(scope, astBase, result)) {
        if (result.size() > 1) {
          diag.error(astBase) << "Ambiguous base type.";
        }
        auto base = result[0];
        auto baseKind = typeKindOf(base);
        switch (td->type()->kind) {
          case Type::Kind::CLASS:
            if (baseKind != Type::Kind::CLASS) {
              diag.error(astBase->location) << "A class can only extend another class.";
            } else if (!td->extends().empty()) {
              diag.error(astBase->location) <<
                  "Multiple implementation inheritance is not allowed.";
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
            diag.error(astBase->location) << "Enumerations cannot be extended.";
            break;
          default:
            assert(false && "Type cannot extend");
        }
        td->extends().push_back(base);
      }
    }

    for (auto astBase : astNode->implements) {
      NameLookupResult result;
      if (resolveDefnName(scope, astBase, result)) {
        if (result.size() > 1) {
          diag.error(astBase) << "Ambiguous base type.";
        }
        auto base = result[0];
        auto baseKind = typeKindOf(base);
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
        td->implements().push_back(base);
      }
    }

    // Classes inherit from 'Object' if no base class specified.
    if (td->type()->kind == Type::Kind::CLASS && td->extends().empty()) {
      td->extends().push_back(intrinsic::IntrinsicDefns::get()->objectClass.get());
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
