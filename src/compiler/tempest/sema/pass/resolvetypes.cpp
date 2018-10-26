#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "tempest/sema/convert/casting.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/eval/evalconst.hpp"
#include "tempest/sema/graph/env.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_lowered.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/infer/conditions.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/overload.hpp"
#include "tempest/sema/infer/paramassignments.hpp"
#include "tempest/sema/infer/solution.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/names/closestname.hpp"
#include "tempest/sema/names/createnameref.hpp"
#include "tempest/sema/names/membernamelookup.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "tempest/sema/transform/visitor.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace tempest::sema::convert;
  using namespace tempest::sema::graph;
  using namespace tempest::sema::names;
  using tempest::sema::infer::CallCandidate;
  using tempest::sema::infer::CallSite;
  using tempest::sema::infer::Conditions;
  using tempest::sema::infer::ContingentType;
  using tempest::sema::infer::InferredType;
  using tempest::sema::infer::OverloadCandidate;
  using tempest::sema::infer::OverloadKind;
  using tempest::sema::infer::ParameterAssignmentsBuilder;
  using tempest::sema::infer::ParamError;
  using tempest::sema::infer::SolutionTransform;
  using tempest::sema::infer::TypeRelation;
  using tempest::sema::transform::ApplySpecialization;
  using tempest::sema::transform::ApplySpecUnique;

  class LowerOperatorsTransform : public transform::ExprVisitor {
  public:
    LowerOperatorsTransform(
        CompilationUnit& cu, LookupScope* scope, support::BumpPtrAllocator& alloc)
      : _cu(cu)
      , _scope(scope)
      , _alloc(alloc)
    {}

    Expr* visit(Expr* e) {
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

    Expr* visitInfixOperator(BinaryOp* op) {
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

    Expr* visitUnaryOperator(UnaryOp* op) {
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

    Expr* evalConstOperator(Expr* op) {
      eval::EvalResult result;
      result.failSilentIfNonConst = true;
      if (eval::evalConstExpr(op, result)) {
        return evalResultToExpr(op->location, result);
      } else if (result.error) {
        return &Expr::ERROR;
      }

      return nullptr;
    }

    Expr* evalResultToExpr(const source::Location& location, eval::EvalResult& result) {
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

    Expr* resolveOperatorName(const Location& loc, const StringRef& name) {
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

  private:
    CompilationUnit& _cu;
    LookupScope* _scope;
    support::BumpPtrAllocator& _alloc;
  };

  // Processing

  void ResolveTypesPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void ResolveTypesPass::process(Module* mod) {
    begin(mod);
    ModuleScope scope(nullptr, mod);
    _scope = &scope;
    visitList(mod->members());
  }

  void ResolveTypesPass::begin(Module* mod) {
    _module = mod;
    _alloc = &mod->semaAlloc();
  }

  // Definitions

  bool ResolveTypesPass::resolve(Defn* defn) {
    if (defn->isResolved()) {
      return true;
    }
    if (defn->isResolving()) {
      diag.error(defn) << "Unable to deduce type signature for " << defn->name();
      return false;
    }
    assert(CompilationUnit::theCU);
    ResolveTypesPass rt(*CompilationUnit::theCU);
    Module* mod = nullptr;
    for (Member* parent = defn->definedIn(); parent; parent = parent->definedIn()) {
      if (parent->kind == Member::Kind::MODULE) {
        mod = static_cast<Module*>(parent);
        break;
      }
    }
    assert(mod);
    rt.begin(mod);
    rt.visitDefn(defn);
    return true;
  }

  void ResolveTypesPass::visitList(DefnArray members) {
    for (auto defn : members) {
      visitDefn(defn);
    }
  }

  void ResolveTypesPass::visitDefn(Defn* d) {
    if (d->isResolved()) {
      return;
    }
    if (d->isResolving()) {
      diag.error(d) << "Unable to deduce type signature for " << d->name();
      return;
    }
    d->setResolving(true);
    switch (d->kind) {
      case Member::Kind::TYPE: {
        visitTypeDefn(static_cast<TypeDefn*>(d));
        break;
      }

      case Member::Kind::FUNCTION: {
        visitFunctionDefn(static_cast<FunctionDefn*>(d));
        break;
      }

      case Member::Kind::VAR_DEF: {
        visitValueDefn(static_cast<ValueDefn*>(d));
        break;
      }

      default:
        assert(false && "Shouldn't get here, bad member type.");
        break;
    }
    d->setResolving(false);
    d->setResolved(true);
  }

  void ResolveTypesPass::visitTypeDefn(TypeDefn* td) {
    auto prevSubject = setSubject(td);
    auto prevScope = _scope;
    TypeParamScope tpScope(_scope, td);
    TypeDefnScope tdScope(&tpScope, td, _cu.spec());
    _scope = &tdScope;

    visitAttributes(td);
    if (td->type()->kind == Type::Kind::ALIAS) {
      // Should be already resolved.
    } else if (td->type()->kind == Type::Kind::ENUM) {
      visitEnumDefn(td);
    } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(td);
    }
    setSubject(prevSubject);
    _scope = prevScope;
  }

  void ResolveTypesPass::visitCompositeDefn(TypeDefn* td) {
    visitList(td->members());
  }

  void ResolveTypesPass::visitEnumDefn(TypeDefn* td) {
    visitList(td->members());
  }

  void ResolveTypesPass::visitFunctionDefn(FunctionDefn* fd) {
    auto prevSubject = setSubject(fd);
    auto prevReturnType = _functionReturnType;
    std::vector<const Type*> prevReturnTypes;
    prevReturnTypes.swap(_returnTypes);
//     savedTempVars = self.tempVarTypes
//     self.tempVarTypes = {}

    visitAttributes(fd);
    for (auto param : fd->params()) {
      visitAttributes(param);
      if (param->init()) {
        assignTypes(param->init(), param->type());
      }
    }

    if (fd->body()) {
      auto prevScope = _scope;
      auto prevSelfType = _selfType;
      FunctionScope fdScope(_scope, fd);
      _scope = &fdScope;

      _selfType = fd->isStatic() ? nullptr : fd->selfType();
      _functionReturnType = fd->type() ? fd->type()->returnType : nullptr;
      if (fd->isConstructor() && !fd->isStatic()) {
        _functionReturnType = &VoidType::VOID;
      }

      LowerOperatorsTransform transform(_cu, _scope, *_alloc);
      auto body = transform.visit(fd->body());
      auto exprType = assignTypes(body, _functionReturnType);

      // If return type was not explicitly specified, infer it from the expression type.
      if (!_functionReturnType) {
        _functionReturnType = chooseIntegerType(exprType);
      }

      // If there were return statements in the function body, and the function return type
      // is not known, compute the minimal return type.
      if (!_returnTypes.empty() && !fd->type()) {
        _returnTypes.push_back(_functionReturnType);
        _functionReturnType = combineTypes(_returnTypes);
      }

      if (diag.errorCount() == 0) {
        // Add in all implicit type casts.
        body = coerceExpr(body, _functionReturnType);
      }

      fd->setBody(body);

      // Compute function type signature from inferred return type.
      if (_functionReturnType && !fd->type()) {
        SmallVector<const Type*, 8> paramTypes;
        for (auto param : fd->params()) {
          paramTypes.push_back(param->type());
        }

        fd->setType(_cu.types().createFunctionType(
            _functionReturnType, paramTypes, fd->isVariadic()));
      }

      _scope = prevScope;
      _selfType = prevSelfType;
    }
//     self.tempVarTypes = savedTempVars
    prevReturnTypes.swap(_returnTypes);
    _functionReturnType = prevReturnType;
    setSubject(prevSubject);
  }

  void ResolveTypesPass::visitValueDefn(ValueDefn* vd) {
    visitAttributes(vd);
    if (vd->init()) {
      LowerOperatorsTransform transform(_cu, _scope, *_alloc);
      vd->setInit(transform.visit(vd->init()));
      auto initType = assignTypes(vd->init(), vd->type());
      if (!Type::isError(initType)) {
        if (!vd->type()) {
          vd->setType(chooseIntegerType(initType));
        }
        vd->setInit(coerceExpr(vd->init(), vd->type()));
        if (vd->init()->kind == Expr::Kind::INTEGER_LITERAL ||
            vd->init()->kind == Expr::Kind::FLOAT_LITERAL) {
          vd->setConstantInit(true);
        }
      }
    }
  }

  void ResolveTypesPass::visitAttributes(Defn* defn) {
    for (auto attr : defn->attributes()) {
      assignTypes(attr, intrinsic::IntrinsicDefns::get()->objectClass.get()->type());
    }
  }

  // Type Inference

  const Type* ResolveTypesPass::assignTypes(Expr* e, const Type* dstType, bool downCast) {
    Location location = e->location;
    ConstraintSolver cs(location);
    // cs = constraintsolver.ConstraintSolver(
    //     location, self, self.typeStore, self.nameLookup.getSubject())
    auto errorCount = diag.errorCount();
    auto exprType = visitExpr(e, cs);
    assert(exprType != nullptr);
    if (errorCount != diag.errorCount()) {
      return &Type::ERROR;
    }

    // TODO: Make sure result is a valid expression, not a type name or non-expression.

    //   cs.renamer.renamerChecker.traverseType(exprType)
    if (dstType != nullptr && dstType->kind != Type::Kind::VOID) {
      if (exprType->kind != Type::Kind::NEVER) {
        cs.addAssignment(e->location, dstType, exprType);
      }
    }
    //   if dstType and exprType is not self.NO_RETURN and dstType is not primitivetypes.VOID:
    //     if exprType is primitivetypes.VOID and not typerelation.isAssignable(dstType, exprType)[0]:
    //       self.errorAt(self.getValueLocation(expr), "Non-void result expected here.")
    //       return graph.ErrorType.defaultInstance()
    //     if downCast:
    //       # If it's a down-cast, then we don't want to static-cast from exprT>ype to dstType because
    //       # that will fail. Instead, we want to determine if there's some supertype of dstType
    //       # that exprType can be converted to.
    //       dstType = self.findCommonDownCastType(expr.getLocation(), dstType, exprType)
    //     dstType = cs.renamer.transformType(dstType)
    //     cs.addAssignment(None, dstType, exprType)
    if (!cs.empty()) {
      return doTypeInference(e, exprType, cs);
    }

    return exprType;
  }

  // Expressions

  const Type* ResolveTypesPass::visitExpr(Expr* e, ConstraintSolver& cs) {
    switch (e->kind) {
      case Expr::Kind::INVALID:
      case Expr::Kind::VOID:
        return &VoidType::VOID;

      case Expr::Kind::BOOLEAN_LITERAL:
        return &BooleanType::BOOL;

      case Expr::Kind::INTEGER_LITERAL:
        return static_cast<IntegerLiteral*>(e)->type;

      case Expr::Kind::FLOAT_LITERAL:
        return static_cast<FloatLiteral*>(e)->type;

      case Expr::Kind::CALL:
        return visitCall(static_cast<ApplyFnOp*>(e), cs);

  // def visitSelf(self, expr, cs):
  //   if isinstance(self.nameLookup.subject, graph.Function) and \
  //       self.nameLookup.subject.hasSelfType():
  //     return cs.renamer.transformType(self.nameLookup.subject.getSelfType())
  //   assert expr.hasType()
  //   return cs.renamer.transformType(expr.getType())

  // def visitSuper(self, expr, cs):
  //   assert expr.hasType()
  //   # TODO - this isn't right, it should probably be some special type
  //   # either that, or we need to tweak the lookup alg.
  //   return cs.renamer.transformType(expr.getType())

      case Expr::Kind::MEMBER_NAME_REF: {
        auto mlist = resolveMemberNameRef(static_cast<MemberNameRef*>(e));
        (void)mlist;
        assert(false && "Implement");
      }

      case Expr::Kind::VAR_REF:
        return visitVarName(static_cast<DefnRef*>(e), cs);

      // case Expr::Kind::FUNCTION_REF:
      //   return visitFunctionName(static_cast<DefnRef*>(e), cs);

      // case Expr::Kind::TYPE_REF:
      //   return visitTypeName(static_cast<DefnRef*>(e), cs);

      case Expr::Kind::BLOCK:
        return visitBlock(static_cast<BlockStmt*>(e), cs);

      case Expr::Kind::LOCAL_VAR:
        return visitLocalVar(static_cast<LocalVarStmt*>(e), cs);

      case Expr::Kind::IF:
        return visitIf(static_cast<IfStmt*>(e), cs);

      case Expr::Kind::WHILE:
        return visitWhile(static_cast<WhileStmt*>(e), cs);

      case Expr::Kind::RETURN: {
        auto ret = static_cast<UnaryOp*>(e);
        if (ret->arg) {
          ret->type = assignTypes(ret->arg, _functionReturnType);
        } else {
          ret->type = &VoidType::VOID;
        }
        _returnTypes.push_back(ret->type);
        return &Type::NO_RETURN;
      }

      case Expr::Kind::ASSIGN:
        return visitAssign(static_cast<BinaryOp*>(e), cs);

      case Expr::Kind::SELF:
        e->type = _selfType;
        return _selfType;

      case Expr::Kind::NOT: {
        auto notOp = static_cast<UnaryOp*>(e);
        assignTypes(notOp->arg, nullptr);
        return &BooleanType::BOOL;
      }

  // def visitThrow(self, expr, cs):
  //   '''@type expr: spark.graph.graph.Throw
  //      @type cs: constraintsolver.ConstraintSolver'''
  //   if expr.hasArg():
  //     self.assignTypes(expr.getArg(), self.typeStore.getEssentialTypes()['throwable'].getType())
  //   return self.NO_RETURN

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(e->kind);
        assert(false && "Invalid expression kind");
    }
  }

  void ResolveTypesPass::visitExprArray(
      llvm::SmallVectorImpl<const Type*>& types,
      const llvm::ArrayRef<Expr*>& exprs,
      ConstraintSolver& cs) {
    for (auto expr : exprs) {
      types.push_back(visitExpr(expr, cs));
    }
  }

  const Type* ResolveTypesPass::visitBlock(BlockStmt* blk, ConstraintSolver& cs) {
    auto prevScope = _scope;
    LocalScope lScope(_scope);
    _scope = &lScope;

    bool earlyExit = false;
    auto errorCount = diag.errorCount();
    for (auto st : blk->stmts) {
      if (earlyExit) {
        diag.error(st) << "Unreachable code.";
      }
      auto sTy = assignTypes(st, nullptr);
      if (diag.errorCount() != errorCount) {
        _scope = prevScope;
        return &Type::ERROR;
      }
      if (auto localVar = dyn_cast<LocalVarStmt>(st)) {
        lScope.addMember(localVar->defn);
      }
      if (sTy->kind == Type::Kind::NEVER) {
        earlyExit = true;
      }
    }

    if (blk->result) {
      if (earlyExit) {
        diag.error(blk->result) << "Unreachable code.";
      }
      auto resultType = assignTypes(blk->result, nullptr);
      _scope = prevScope;
      return resultType;

    }

    _scope = prevScope;
    return earlyExit ? &Type::NO_RETURN : &VoidType::VOID;
  }

  const Type* ResolveTypesPass::visitLocalVar(LocalVarStmt* expr, ConstraintSolver& cs) {
    auto vd = expr->defn;
    if (vd->init()) {
      auto initType = assignTypes(vd->init(), vd->type());
      if (!Type::isError(initType)) {
        if (!vd->type()) {
          vd->setType(chooseIntegerType(initType));
        }
        vd->setInit(coerceExpr(vd->init(), vd->type()));
      }
    } else if (!vd->type()) {
      diag.error(expr) << "Local definition has no type and no initializer.";
    }

    expr->type = &Type::NOT_EXPR;
    return &Type::NOT_EXPR;
  }

  const Type* ResolveTypesPass::visitIf(IfStmt* expr, ConstraintSolver& cs) {
    // Coerce test to bool later.
    expr->test = booleanTest(expr->test);
    auto thenType = visitExpr(expr->thenBlock, cs);
    if (expr->elseBlock) {
      auto elseType = visitExpr(expr->elseBlock, cs);
      if (elseType->kind == Type::Kind::NEVER) {
        return thenType;
      } else if (thenType->kind == Type::Kind::NEVER) {
        return elseType;
      } else if (isAssignable(thenType, elseType).rank > ConversionRank::WARNING) {
        return thenType;
      } else if (isAssignable(elseType, thenType).rank > ConversionRank::WARNING) {
        return elseType;
      } else {
        return _cu.types().createUnionType({ thenType, elseType });
      }
    } else {
      if (thenType->kind == Type::Kind::NEVER) {
        return thenType;
      } else if (thenType->kind != Type::Kind::VOID) {
        return _cu.types().createUnionType({ thenType, &VoidType::VOID });
      }
      return thenType;
    }
  }

  const Type* ResolveTypesPass::visitWhile(WhileStmt* expr, ConstraintSolver& cs) {
    // Coerce to bool later.
    expr->test = booleanTest(expr->test);
    assignTypes(expr->body, nullptr);
    return &VoidType::VOID;
  }

  const Type* ResolveTypesPass::visitAssign(BinaryOp* expr, ConstraintSolver& cs) {
    auto lType = visitExpr(expr->args[0], cs);
    auto rType = visitExpr(expr->args[1], cs);
    cs.addAssignment(expr->location, lType, rType);
    return &Type::NOT_EXPR;
  }

  const Type* ResolveTypesPass::visitCall(ApplyFnOp* expr, ConstraintSolver& cs) {
    if (expr->function->kind == Expr::Kind::MEMBER_NAME_REF) {
      // If it's a member name reference (expression.name), then replace the function with
      // the list of possible members.
      auto fn = resolveMemberNameRef(static_cast<MemberNameRef*>(expr->function));
      if (Expr::isError(fn)) {
        return &Type::ERROR;
      }
      expr->function = fn;
    }

    switch (expr->function->kind) {
      case Expr::Kind::TYPE_REF_OVERLOAD:
      case Expr::Kind::FUNCTION_REF_OVERLOAD: {
        return visitCallName(expr, expr->function, expr->args, cs);
        break;
      }
      case Expr::Kind::SUPER: {
        auto subjectFn = dyn_cast_or_null<FunctionDefn>(_subject);
        if (!subjectFn) {
          diag.error(expr) << "Can't call 'super' outside of a method.";
          return &Type::ERROR;
        }
        if (subjectFn->isStatic()) {
          diag.error(expr) << "Can't call 'super' from a static method.";
          return &Type::ERROR;
        }
        auto baseClsDef = cast<UserDefinedType>(_selfType)->defn();
        MemberNameLookup lookup(_cu.spec());
        MemberLookupResult lookupResult;
        lookup.lookup(subjectFn->name(), baseClsDef, lookupResult,
            MemberNameLookup::INHERITED_ONLY | MemberNameLookup::INSTANCE_MEMBERS);
        if (lookupResult.empty()) {
          diag.error(expr) << "No inherited method named '" << subjectFn->name() << "'.";
          return &Type::ERROR;
        }

        SmallVector<const Type*, 8> argTypes;
        visitExprArray(argTypes, expr->args, cs);
        for (auto t : argTypes) {
          if (Type::isError(t)) {
            return &Type::ERROR;
          }
        }

        return addCallSite(expr, expr->function, lookupResult, expr->args, argTypes, cs);
      }

      default: {
        assert(false && "Implement");
        break;
      }
    }
  }

//   def visitCall(self, callExpr, cs):
//     '''@type callExpr: spark.graph.Call
//        @type cs: constraintsolver.ConstraintSolver'''
//     funcExpr, *args = callExpr.getArgs()
//     for a in args:
//       assert isinstance(a, graph.Expr)
//     assert isinstance(funcExpr, graph.Expr)

//     if isinstance(funcExpr, (graph.MemberList, graph.ExplicitSpecialize)):
//       return self.visitCallName(callExpr, funcExpr, args, cs)
//     elif isinstance(funcExpr, graph.Super):
//       if not self.nameLookup.subject:
//         self.errorAt(funcExpr, "Can't call 'super' outside of a method.")
//         return graph.ErrorType.defaultInstance()
//       mlist = graph.MemberList().setLocation(funcExpr.getLocation())
//       mlist.setName(self.nameLookup.subject.getName())
//       mlist.setListType(graph.MemberListType.INCOMPLETE)
//       mlist.setBase(funcExpr)
//       callExpr.getArgs()[0] = mlist
//       return self.visitCallName(callExpr, mlist, args, cs)
//     elif isinstance(funcExpr, graph.Call):
//       argTypes = self.traverseExprList(args, cs)
//       return self.visitCallExpr(callExpr, funcExpr, args, argTypes, cs)
//     elif funcExpr.typeId() == graph.ExprType.DEFN_REF:
//       assert False, 'Should not exist yet'
//     else:
//       resultType = self.assignTypes(funcExpr, None)
//       assert False, debug.format(funcExpr, type(funcExpr), resultType)
//     assert False

  const Type* ResolveTypesPass::visitCallName(
      ApplyFnOp* callExpr, Expr* fn, const ArrayRef<Expr*>& args, ConstraintSolver& cs) {

    // Gather the types of all arguments.
    SmallVector<const Type*, 8> argTypes;
    visitExprArray(argTypes, args, cs);
    for (auto t : argTypes) {
      if (Type::isError(t)) {
        return &Type::ERROR;
      }
    }
    // for (size_t i = 0; i < argTypes.size(); i += 1) {
    //   if (args[i]->kind == Expr::Kind::INTEGER_LITERAL) {
    //     argTypes[i] = chooseIntegerType(args[i], argTypes[i]);
    //   }
    // }

    if (fn->kind == Expr::Kind::FUNCTION_REF_OVERLOAD) {
      auto mref = static_cast<MemberListExpr*>(fn);
      // TODO: ADL lookup.
      if (mref->members.empty()) {
        if (mref->isOperator) {
          diag.error(fn) << "Operator '" << mref->name << "' not found.";
        } else {
          diag.error(fn) << "Method '" << mref->name << "' not found.";
        }
        return &Type::ERROR;
      }
      return addCallSite(callExpr, fn, mref->members, args, argTypes, cs);
    } else if (fn->kind == Expr::Kind::TYPE_REF_OVERLOAD) {
      auto mref = static_cast<MemberListExpr*>(fn);
      MemberLookupResult ctors;
      findConstructors(mref->members, ctors);
      if (ctors.empty()) {
        diag.error(fn) << "No constructor found for type  '" << mref->name << "'.";
        return &Type::ERROR;
      }
      return addCallSite(callExpr, fn, ctors, args, argTypes, cs);
    }

//     listType = func.getListType()
//     listType == graph.MemberListType.VARIABLE:
//       assert len(func.getMembers()) == 1
//       var = func.getMembers()[0]
//       if isinstance(defns.raw(var).getType(), graph.FunctionType):
//         return self.visitCallExpr(callExpr, func, args, argTypes, cs)
//       func = self.findCallMethod(func)
//       if self.isErrorExpr(func):
//         return graph.ErrorType.defaultInstance()
//       callExpr.getMutableArgs()[0] = func

    assert(false && "Implement");
  }

  void ResolveTypesPass::findConstructors(
      const ArrayRef<MemberAndStem>& members,
      MemberLookupResultRef& ctors) {
    MemberNameLookup lookup(_cu.spec());
    lookup.lookup("new", members, ctors);
  }

  const Type* ResolveTypesPass::addCallSite(
      ApplyFnOp* callExpr,
      Expr* fn,
      const ArrayRef<MemberAndStem>& methodList,
      const ArrayRef<Expr*>& args,
      const ArrayRef<const Type*>& argTypes,
      ConstraintSolver& cs) {
    auto site = new CallSite(callExpr->location, callExpr, args, argTypes);
    cs.addSite(site);

    // Record whether the call was generated by an operator, used for tailoring error messages.
    if (auto mref = dyn_cast<MemberListExpr>(fn)) {
      site->isOperator = mref->isOperator;
    }

    // The return type will depend on which overload type gets chosen.
    SmallVector<ContingentType::Entry, 8> returnTypes;

    // For each possible function that could have been called.
    for (auto method : methodList) {
      auto cc = site->addCandidate(method.member, method.stem);

      // Collect explicit type arguments
      std::unordered_map<TypeParameter*, const Type*> explicitTypeArgs;
      if (method.member->kind == Member::Kind::SPECIALIZED) {
        auto sp = static_cast<SpecializedDefn*>(method.member);
        for (size_t i = 0; i < sp->typeParams().size(); i += 1) {
          auto arg = sp->typeArgs()[i];
          auto param = sp->typeParams()[i];
          if (arg) {
            explicitTypeArgs[param] = arg;
          }
        }
        method.member = sp->generic();
      }

      // Make sure function type is resolved.
      auto function = cast<FunctionDefn>(method.member);
      auto params = function->params();
      if (!function->type()) {
        if (!resolve(function)) {
          return &Type::ERROR;
        }
      }
      auto returnType = function->type()->returnType;
      if (function->isConstructor()) {
        returnType = function->selfType();
        assert(returnType);
      }

      // If it's a generic
      if (function->allTypeParams().size() > 0) {
        // Make an environment and map the function type through it.
        // Three kinds of type arguments
        // * explicit - type arguments that are explicitly provided by specialization.
        // * inferred - type arguments that need to be deduced by type solver
        // * outer - type parameters that should not be bound, because they are
        //     part of the enclosing template context of the current function.
        cc->typeArgs.resize(function->allTypeParams().size(), nullptr);
        for (size_t i = 0; i < cc->typeArgs.size(); i += 1) {
          auto typeParam = function->allTypeParams()[i];
          auto it = explicitTypeArgs.find(typeParam);
          if (it != explicitTypeArgs.end()) {
            // Argument was already explicitly mapped.
            cc->typeArgs[i] = it->second;
          } else if (!_subject || !_subject->hasTypeParam(typeParam)) {
            // Don't infer params from the enclosing scope.
            auto inferred = new InferredType(typeParam, &cs);
            cc->typeArgs[i] = inferred;
          }
        }

        Env env;
        env.params = function->allTypeParams();
        env.args = cc->typeArgs;
        llvm::SmallVector<const Type*, 8> mappedParamTypes;
        ApplySpecialization transform(env.args);
        transform.transformArray(mappedParamTypes, function->type()->paramTypes);
        cc->paramTypes = cs.alloc().copyOf(mappedParamTypes);
        returnType = transform.transform(returnType);

        // Add constraints on type arguments
        for (size_t i = 0; i < cc->typeArgs.size(); i += 1) {
          auto typeParam = function->allTypeParams()[i];
          auto typeArg = cc->typeArgs[i];
          for (auto subtype : typeParam->subtypeConstraints()) {
            cs.addExplicitConstraint(
                typeParam, typeArg, transform.transform(subtype), TypeRelation::SUBTYPE, cc);
          }
        }
      } else {
        cc->paramTypes = function->type()->paramTypes;
      }

//       # Why is super.T not inferred?
//       # We're calling the method 'new' from the superclass, whose first argument is type T.
//       # The subclass inherits from superclass[Object], meaning that T is bound to Object.
//       # How do we find this out? We need to know that the method is inherited from an instantiated
//       # supertype.

//       # Relabel all type variables in the parameter types, the return types, and the instantiation
//       # mapping. These should all have local labels.
//       ccTypeVars = {}
//       renamedMappedVars = {}
//       renamingMap = {}
//       if inferredParams or outerParams or mappedVars:
//         ambiguousTypeTransform = callsite.LabelAmbiguousTypes(self.typeStore, cs, cc)
//         for tparam in inferredParams:
//           # Translate references from the unlabeled parameter into a locally-labeled one.
//           tvar = cs.renamer.createTypeVar(tparam, cc)
//           renamingMap[tparam.getTypeVar()] = tvar
//           ccTypeVars[tparam.getTypeVar()] = tvar
//           for st in tparam.getSubtypeConstraints():
//             cs.addSubtypeConstraint(None, tvar, st, cc)
//         for tparam in outerParams:
//           # If the outer param is not explicitly mapped...(TODO: This logic should be moved
//           # up to where we compute the outer params).
//           if tparam.getTypeVar() not in mappedVars:
//             # Outer params are type parameters which are inherited from the enclosing scope
//             # rather than inferred or explicitly mapped. They can only exist in cases where the
//             # call expression and the called function share a common ancestor scope. Outer params
//             # are not locally-labeled (the second parameter below is None) because all call
//             # candidates share the same value for the parameter.
//             # (Except that this causes unit tests to fail, so I reverted it...)
//             tvar = cs.renamer.createTypeVar(tparam, cc)
//             assert tvar not in mappedVars
//             if tvar not in mappedVars:
//               renamingMap[tparam.getTypeVar()] = tvar
//               ccTypeVars[tparam.getTypeVar()] = tvar
//               # For outer params, add an equivalence to the original unbound type parameter.
//               cs.addEquivalence(tparam.getLocation(), tvar, tparam.getTypeVar())
// #           for st in tparam.getSubtypeConstraints():
// #             cs.addSubtypeConstraint(None, tvar, st, cc)
//         for key, value in mappedVars.items():
//           assert not key.hasIndex()
//           # Translate references from the unlabeled parameter into a locally-labeled one,
//           # and also add an explicit equivalence constraint.
//           loc = key.getParam().getLocation()
//           tvar = cs.renamer.createTypeVar(key.getParam(), cc)
//           renamedValue = renamedMappedVars[tvar] = ambiguousTypeTransform.traverseType(
//               cs.renamer.transformType(value))
//           renamingMap[key] = tvar
//           cs.addEquivalence(loc, tvar, renamedValue, cc)
//           cs.locateAdditionalConstraints(loc, renamedValue, cc)

//         # Handle templated 'self' parameter.
//         if method.hasSelfType() and isinstance(method.getSelfType(), graph.TypeVar):
//           assert isinstance(func, graph.MemberList)
//           assert func.hasBaseType()
//           selfArgType = func.getBaseType()
//           tvar = cs.renamer.createTypeVar(method.getSelfType().getParam(), cc)
//           renamedMappedVars[tvar] = cs.renamer.transformType(selfArgType)
//           renamingMap[method.getSelfType()] = tvar
//           cs.addEquivalence(None, tvar, selfArgType, cc)

//         # Rename all type variables: If they are an inferred variable, then give them a local
//         # label, otherwise give them a global label.
//         renameTransform = renamer.ApplyTypeMapTransform(environ.Env(renamingMap), cs.renamer)

//         # Handle type parameter defaults
//         for de in defns.ancestorDefs(method):
//           for tparam in de.getTypeParams():
//             if tparam not in outerParams:
//               tvar = tparam.getTypeVar()
//               if tvar not in outerParams and tvar not in mappedVars and tparam.hasDefaultType():
//                 tvar = cs.renamer.createTypeVar(tparam, cc)
//                 value = renameTransform.traverseType(tparam.getDefaultType())
//                 value = ambiguousTypeTransform.traverseType(value)
//                 cs.addEquivalence(None, tvar, value, cc)
//                 cs.locateAdditionalConstraints(tparam.getLocation(), value, cc)

//         params = renameTransform.traverseParamList(params)
//         paramTypes = renameTransform.traverseTypeList(paramTypes)
//         returnType = renameTransform.traverseType(returnType)
// #         debug.write('returnType', returnType)
// #         debug.write('ccTypeVars', ccTypeVars)
// #         debug.write('renamingMap', renamingMap)
//         cc.setRenameTransform(renameTransform)
//       else:
//         params = cs.renamer.transformParams(params)
//         paramTypes = cs.renamer.transformTypeList(paramTypes)
//         returnType = cs.renamer.transformType(returnType)

//       # TODO: Something about this logic is wrong - this shouldn't be necessary.
//       cs.renamer.renamerChecker.traverseParamList(params)
//       cs.renamer.renamerChecker.traverseType(returnType)

      cc->params = params;
      cc->returnType = returnType;
      cc->isVariadic = function->type()->isVariadic;

      ParameterAssignmentsBuilder builder(cc->paramAssignments, cc->params, cc->isVariadic);
      size_t argIndex = 0;
      for (auto arg : args) {
        if (builder.error() != ParamError::NONE) {
          break;
        }
        // TODO: Keyword args.
        (void)arg;
        // if (arg->kind == Expr::Kind::KEY)
        builder.addPositionalArg();
        argIndex += 1;
      }
      builder.validate();

      if (builder.error() != ParamError::NONE) {
        if (builder.error() == ParamError::POSITIONAL_AFTER_KEYWORD) {
          diag.error(callExpr) << "Keyword arguments must come after all position arguments.";
          return &Type::ERROR;
        }
        cc->rejection = builder.rejection();
        cc->rejection.argIndex = argIndex;
      }

//       cc.setTypeVars(environ.Env(ccTypeVars))
//       # TODO: Actually what this should be testing is explicit type params. We don't want to
//       # add requirements from implicit params, since it's presumed that they are already satisifed.
//       if listType == graph.MemberListType.TYPE:
//         for defn in defns.ancestorDefs(method):
//           if defn not in cs.outerScopes:
//             cs.addRequirements(defn.getRequirements(), (environ.Env(renamingMap),), cc)

//       if cc.rejection:
//         continue

//       # Callers need a concrete type to do ADL lookup.
//       if mappedVars:
//         returnType = self.canonicalize.traverseType(returnType, environ.Env(renamedMappedVars))

      returnTypes.push_back({ cc, returnType });
    }

    // Compute the combined return type
    const Type* commonReturnType = nullptr;
    for (auto tr : returnTypes) {
      if (commonReturnType) {
        if (commonReturnType != tr.type) {
          commonReturnType = nullptr;
          break;
        }
      } else {
        commonReturnType = tr.type;
      }
    }

    if (commonReturnType) {
      return commonReturnType;
    }

    // Return ambiguous result.
    assert(!returnTypes.empty());
    return new (cs.alloc()) ContingentType(cs.alloc().copyOf(returnTypes));
  }

//   def lookupSpecializedCallable(self, func, argTypes, cs):
//     assert isinstance(func.getBase(), graph.MemberList)
//     mlist = self.lookupCallable(func.getBase(), argTypes, cs)
//     if self.isErrorExpr(mlist):
//       return mlist
//     assert len(mlist.getMembers()) > 0
//     members = mlist.getMembers()
//     typeArgs = func.getTypeArgs()

//     listType = mlist.getListType()
//     elif listType == graph.MemberListType.VARIABLE:
//       self.errorAtFmt(mlist.getLocation(), "Variable '{0}' cannot be specialized.", mlist)
//       return graph.ErrorType.defaultInstance()

//     templatesWithMatchingParams = []
//     for m in members:
//       env, m, _ = self.graphBuilder.mapTemplateArgs(m, typeArgs)
//       if env:
//         templatesWithMatchingParams.append((m, env))

//     if not templatesWithMatchingParams:
//       if len(members) == 1:
//         m = members[0]
//         _, _, errorMsg = self.graphBuilder.mapTemplateArgs(m, typeArgs)
//         self.errorAt(mlist, errorMsg)
//       else:
//         self.errorAt(func, "No template found which matches specialization arguments.")
//         self.info("Candidates are:")
//         for m in members:
//           _, _, errorMsg = self.graphBuilder.mapTemplateArgs(m, typeArgs)
//           self.infoAt(m, errorMsg)
//           return graph.ErrorType.defaultInstance()
//       return graph.ErrorType.defaultInstance()

//     templatesWithMatchingRequirements = []
//     methodFinder = requiredmethodfinder.RequiredMethodFinder(func.getLocation(), self.typeStore)
//     for m, env in templatesWithMatchingParams:
//       fulfillments = methodFinder.findMethodsFor(m, env)
//       assert len(fulfillments) == 0, 'Add fulfillments to local cache'
//       if fulfillments is not None:
//         templatesWithMatchingRequirements.append((m, env, fulfillments))

//     if len(templatesWithMatchingRequirements) == 0:
//       self.errorAtFmt(func, "No matching template found for explicit specialization '{0}'.", func)
//       assert False, 'List candidates'
//       return graph.ErrorType.defaultInstance()

//     mlist.getMutableMembers().clear()
//     for m, env, fulfillments in templatesWithMatchingRequirements:
//       mlist.getMutableMembers().append(graph.InstantiatedMember().setBase(m).setEnv(env))
//     return mlist

//   def lookupCallable(self, func, argTypes, cs):
//     members = func.getMembers()
//     assert len(members) == 0, debug.format(func, argTypes)
//     if func.hasBase():
//       try:
//         if not self.lookupMembersByName(func, cs, self.nameLookup.nameNotFound):
//           return graph.Error.defaultInstance()
//         return func
//       except:
//         raise AssertionError(debug.format('looking up', func))
//     else:
//       methods = []
//       self.findMethodsFromArgumentTypes(func.getLocation(), func.getName(), argTypes, methods)
//       if len(methods) == 0:
//         return graph.ErrorType.defaultInstance()
//       methods = [m for m in methods if defns.isStaticDefn(m)]
//       if len(methods) == 0:
//         self.errorAtFmt(func.getLocation(),
//             "No static function '{0}' found for argument types.", func.getName())
//         return graph.ErrorType.defaultInstance()
//       mlist = self.nameLookup.createMemberList(
//           func.getLocation(), None, func.getName(), methods)
//       return mlist

//   def findCallMethod(self, valueExpr):
//     # Look for a nameless method
//     assert len(valueExpr.getMembers()) == 1
//     var = valueExpr.getMembers()[0]
//     varScopes = self.nameLookup.getScopesForMember(var)
//     callables = []
//     for s in varScopes:
//       callables.extend(s.lookupName("__call__"))
//     if len(callables) == 0:
//       self.errorAtFmt(valueExpr.getLocation(), "Expression '{0}' is not callable.", valueExpr)
//       return graph.ErrorType.defaultInstance()
//     mlist = self.nameLookup.createMemberList(
//         valueExpr.getLocation(), valueExpr, '__call__', callables)
// #         print(type(callables[0].getBase()), mlist.getListType(), len(callables))
//     if self.isErrorExpr(mlist):
//       return mlist
//     if mlist.getListType() == graph.MemberListType.VARIABLE:
//       assert len(mlist.getMembers()) == 1
//       first = mlist.getMembers()[0]
//       if first.typeId() == graph.MemberKind.INSTANTIATED:
//         first = first.getBase()
//       if first.typeId() != graph.MemberKind.PROPERTY:
//         self.errorAtFmt(valueExpr.getLocation(),
//             "Expression '{0}' is not callable (no).", valueExpr)
//         print('first:', type(first))
//         return graph.ErrorType.defaultInstance()
//     elif mlist.getListType() != graph.MemberListType.FUNCTION:
//       self.errorAtFmt(valueExpr.getLocation(),
//           "Expression '{0}' is not callable (no funcs).", valueExpr)
//       return graph.ErrorType.defaultInstance()
//     return mlist

//   def findCallMethodForType(self, loc, ty, baseExpr):
//     # Look for a nameless method
//     varScopes = self.nameLookup.getScopesForType(ty)
//     callables = []
//     for s in varScopes:
//       callables.extend(s.lookupName("__call__"))
//     if len(callables) == 0:
//       self.errorAtFmt(loc, "Type '{0}' is not callable.", ty)
//       return graph.ErrorType.defaultInstance()
//     mlist = self.nameLookup.createMemberList(
//         loc, baseExpr, '__call__', callables)
//     if self.isErrorExpr(mlist):
//       return mlist
//     if mlist.getListType() == graph.MemberListType.VARIABLE:
//       assert len(mlist.getMembers()) == 1
//       first = mlist.getMembers()[0]
//       if first.typeId() == graph.MemberKind.INSTANTIATED:
//         first = first.getBase()
//       if first.typeId() != graph.MemberKind.PROPERTY:
//         self.errorAtFmt(loc,
//             "Type '{0}' is not callable.", ty)
//         print('first:', type(first))
//         return graph.ErrorType.defaultInstance()
//     elif mlist.getListType() != graph.MemberListType.FUNCTION:
//       self.errorAtFmt(loc,
//           "Type '{0}' is not callable.", ty)
//       return graph.ErrorType.defaultInstance()
//     return mlist

//   def visitCallExpr(self, callExpr, func, args, argTypes, cs):
//     fntype = self.assignTypes(func, None)
//     if types.isError(fntype):
//       return fntype

//     if isinstance(types.raw(fntype), graph.Composite):
//       mlist = self.findCallMethodForType(func.getLocation(), fntype, func)
//       if self.isErrorExpr(mlist):
//         return mlist
//       callExpr.getArgs()[0] = mlist
//       return self.addCallSite(callExpr, mlist, graph.MemberListType.VARIABLE, args, argTypes, cs)

//     assert isinstance(fntype, graph.FunctionType)

//     site = callsite.CallSite(callExpr, args, argTypes)
//     cs.addSite(site)

//     #@type cc: CallCandidate
//     cc = site.addExprCandidate(func)
//     cc.setReturnType(cs.renamer.transformType(fntype.getReturnType()))
//     cc.setParamTypes([cs.renamer.transformType(t) for t in fntype.getParamTypes()])

//     if not cc.rejection:
//       for argIndex, paramIndex in enumerate(cc.paramAssignments.paramIndex):
//         argType = argTypes[argIndex]
//         paramType = cc.paramTypes[paramIndex]
//         cs.addAssignment(args[argIndex].getLocation(), paramType, argType, cc)

//     return cc.returnType

  const Type* ResolveTypesPass::visitVarName(DefnRef* expr, ConstraintSolver& cs) {
    auto vd = cast<ValueDefn>(expr->defn);
    if (!vd->type()) {
      assert(false && "Do eager type resolution");
    }
    return vd->type();
  }

  const Type* ResolveTypesPass::visitFunctionName(DefnRef* expr, ConstraintSolver& cs) {
    assert(false && "Implement");
  }

  const Type* ResolveTypesPass::visitTypeName(DefnRef* expr, ConstraintSolver& cs) {
    assert(false && "Implement");
  }

  // Type Inference

  const Type* ResolveTypesPass::doTypeInference(
      Expr* expr, const Type* exprType, ConstraintSolver& cs) {
//   def doTypeInference(self, expr, exprType, cs):
//     try:
//       solutionEnv = None
//       # TODO: It would be nice to have an optimized run if there are no type vars involved.
//       if self.nameLookup.getSubject().getName() in TRACE_SUBJECTS:
//         debug.tracing = True
//         cs.tracing = True
    cs.run();
    if (cs.failed()) {
      return &Type::ERROR;
    }

    SolutionTransform transform(*_alloc);
    applySolution(cs, transform);

//         if exprType:
//           finalExprType = typesubstitution.ApplyEnv(solutionEnv).traverseType(exprType)
//           finalExprType = callsite.CollapseAmbiguousTypes(
//               self.typeStore).traverseType(finalExprType)
//           return finalExprType
    return transform.transform(exprType);
  }

  void ResolveTypesPass::applySolution(ConstraintSolver& cs, SolutionTransform& st) {
    for (auto site : cs.sites()) {
      if (site->kind == OverloadKind::CALL) {
        CallSite* c = static_cast<CallSite*>(site);
        updateCallSite(cs, c, st);
      }
//       mlist = site.callExpr.getArgs()[0]
//       if method:
//         assert isinstance(method, graph.Member), debug.write(method)
//         assert isinstance(mlist, graph.MemberList), type(mlist)
//         mlist.setMembers([method])
//         mlist.setListType(graph.MemberListType.FUNCTION)
//         if not mlist.hasBase() and needsBaseExpr:
//           self.errorAtFmt(site.callExpr.getLocation(),
//               "Call to instance method '{0}' from static context", method.getName())
//         if mlist.hasBase():
//           base = mlist.getBase()

//           # Replace the environment in the method base expression.
//           if isinstance(base, graph.MemberList) and base.getListType() == graph.MemberListType.TYPE:
//             if isinstance(method, graph.InstantiatedMember):
//               definedIn = method.getBase().hasDefinedIn() and method.getBase().getDefinedIn()
//             else:
//               definedIn = method.hasDefinedIn() and method.getDefinedIn()
//             baseMemberList = []
//             if len(base.getMembers()) > 1:
//               # TODO: Incorrect. We need to reduce the list of base members to the member that
//               # defined the chosen method; using 'definedIn' works only when the chosen method
//               # was not defined in some base class of the original type that was searched.
//               # The reason we don't do this is because the location of where the method was found
//               # is not currently preserved.
//               assert definedIn
//               baseMemberList.append(cs.applyEnv(definedIn, (methodEnv,)))
//             else:
//               for m in base.getMembers():
//                 if isinstance(m, graph.InstantiatedMember) and m.getEnv() != methodEnv:
//                   m = cs.applyEnv(m.getBase(), (methodEnv,))
//                 else:
//                   m = cs.applyEnv(m, (methodEnv,))
//                 baseMemberList.append(m)
//             if baseMemberList != base.getMembers():
//               base = base.shallowCopy().setMembers(baseMemberList)
//           elif transform:
//             base = transform.traverseExpr(base)

//           mlist.setBase(base)
//           renamer.EnsureAllTypeVarsAreNormalized().traverseExpr(mlist.getBase())
    }
  }

  void ResolveTypesPass::updateCallSite(
      ConstraintSolver& cs, CallSite* site, SolutionTransform& st) {
    auto candidate = static_cast<CallCandidate*>(site->singularCandidate());
    auto callExpr = static_cast<ApplyFnOp*>(site->callExpr);
    FunctionDefn* fn = cast<FunctionDefn>(unwrapSpecialization(candidate->method));
    const Type* fnType = fn->type();
    const Type* returnType = fn->type()->returnType;
    const Type* fnSelfType = fn->selfType();
    Member* method = fn;
    if (candidate->typeArgs.size() > 0) {
      llvm::SmallVector<const Type*, 8> typeArgs;
      st.transformArray(typeArgs, candidate->typeArgs);
      ApplySpecUnique asu(_cu.types(), _cu.spec(), typeArgs);
      fnType = asu(fnType);
      returnType = asu(returnType);
      fnSelfType = asu(fnSelfType);
      method = _cu.spec().specialize(cast<GenericDefn>(method), typeArgs);
    }

    // diag.debug() << "fn: " << fnType << " rt: " << returnType << " " << method;

    // Patch the call expression with a new callable
    if (callExpr->function->kind == Expr::Kind::FUNCTION_REF_OVERLOAD) {
      // Create a new signular function reference.
      auto fnRef = static_cast<MemberListExpr*>(callExpr->function);
      // If the function reference had an explicit stem ('a.x()') then use it, otherwise
      // preserve the implicit stem ('self');
      auto stem = fnRef->stem ? fnRef->stem : candidate->stem;
      callExpr->function = new (*_alloc) DefnRef(
          Expr::Kind::FUNCTION_REF, site->location, method, stem, fnType);
      callExpr->type = returnType;
      callExpr->flavor = ApplyFnOp::STATIC;
      if (stem) {
        callExpr->flavor = ApplyFnOp::METHOD;
      }
    } else if (callExpr->function->kind == Expr::Kind::TYPE_REF_OVERLOAD) {
      // Create a new singular constructor reference.
      auto allocObj = new (*_alloc) SymbolRefExpr(
          Expr::Kind::ALLOC_OBJ, callExpr->location, nullptr, fnSelfType);
      callExpr->function = new (*_alloc) DefnRef(
          Expr::Kind::FUNCTION_REF, site->location, method, allocObj, fnSelfType);
      callExpr->type = fnSelfType;
      callExpr->flavor = ApplyFnOp::NEW;
      if (auto udSelf = dyn_cast<UserDefinedType>(fn->selfType())) {
        if (udSelf->defn()->isFlex()) {
          callExpr->flavor = ApplyFnOp::FLEXNEW;
        }
      }
    } else if (callExpr->function->kind == Expr::Kind::SUPER) {
      // Create a new signular function reference.
      auto selfArg = new (*_alloc) SelfExpr(callExpr->location, _selfType);
      callExpr->function = new (*_alloc) DefnRef(
          Expr::Kind::FUNCTION_REF, site->location, method, selfArg, fnType);
      callExpr->type = returnType;
      callExpr->flavor = ApplyFnOp::SUPER;
    } else {
      assert(false && "Implement other callable types");
    }

    reorderCallingArgs(cs, st, callExpr, site, candidate);
  }

// #     if leftOverVars:
// #       assert False, debug.format('Unused solution vars:', environ.Env(leftOverVars))

//     # Add template arguments to generic members that were referenced without any instantiation.
//     if cs.genericMemberRefs or cs.anonFns:
//       transformMap = {}
//       for inferredVar in nonContextualTypeVars:
//         explicitVar = inferredVar.getParam().getTypeVar()
//         assert explicitVar is not inferredVar
//         if inferredVar in solutionEnv:
//           value = solutionEnv[inferredVar]
//           if explicitVar not in transformMap and explicitVar is not value:
//             transformMap[explicitVar] = value

//       transformEnv = environ.Env(transformMap)
//       for mlist in cs.genericMemberRefs:
//         members = []
//         for member in mlist.getMembers():
//           if isinstance(member, (graph.ValueDefn, graph.Function, graph.Property)):
//             typeParams = defns.getTypeParamsForDefn(member)
//             if len(typeParams) > 0:
//               member = cs.applyEnv.visit(member, (transformEnv,))
//           members.append(member)
//         mlist.getMutableMembers()[:] = members

//       for anonFn in cs.anonFns:
//         func = anonFn.getFunc()
//         for param in func.getParams():
//           param.setType(cs.applyEnv.visit(param.getType(), (transformEnv,)))
//         func.getType().getMutableParamTypes()[:] = [p.getType() for p in func.getParams()]
//         func.getType().setReturnType(
//             cs.applyEnv.visit(func.getType().getReturnType(), (transformEnv,)))
//         self.assignTypes(func.getBody(), func.getType().getReturnType())

  void ResolveTypesPass::reorderCallingArgs(
      ConstraintSolver& cs, SolutionTransform& st,
      ApplyFnOp* callExpr, CallSite* site, CallCandidate* cc) {
    size_t argIndex = 0;
    SmallVector<Expr*, 8> argList;
    SmallVector<Expr*, 8> varArgList;
    argList.resize(cc->params.size(), nullptr);

    // Assign explicit arguments
    for (auto arg : site->argList) {
      size_t paramIndex = cc->paramAssignments[argIndex++];
      auto paramType = st.transform(cc->paramTypes[paramIndex]);
      if (cc->isVariadic && paramIndex == cc->paramTypes.size() - 1) {
        arg = coerceExpr(arg, paramType);
        varArgList.push_back(arg);
      } else {
        arg = coerceExpr(arg, paramType);
        argList[paramIndex] = arg;
      }
    }

    // Handle rest args
    if (cc->isVariadic) {
      // TODO: This needs to be an array or slice type
      MultiOp* restArgs = new (*_alloc) MultiOp(
          Expr::Kind::REST_ARGS, callExpr->location, _alloc->copyOf(varArgList),
          cc->paramTypes.back());
      argList[cc->paramTypes.size() - 1] = restArgs;
    }

    // Now, fill in defaults
    for (size_t i = 0; i < cc->params.size(); i += 1) {
      if (argList[i] == nullptr) {
        auto param = cc->params[i];
        if (param->init()) {
          // TODO: Transform
          argList[i] = param->init();
        }
      }
    }

    callExpr->args = _alloc->copyOf(argList);
  }

  // Coerce types

  Expr* ResolveTypesPass::coerceExpr(Expr* e, const Type* dstType) {
    if (e == nullptr) {
      return e;
    }

    switch (e->kind) {
      case Expr::Kind::VOID:
        e->type = &VoidType::VOID;
        return e;

      case Expr::Kind::BOOLEAN_LITERAL:
        e->type = &BooleanType::BOOL;
        return e;

      case Expr::Kind::SELF:
      case Expr::Kind::SUPER:
      case Expr::Kind::ALLOC_OBJ:
        assert(e->type);
        return addCastIfNeeded(e, dstType);

      case Expr::Kind::INTEGER_LITERAL: {
        assert(e->type);
        auto intLit = static_cast<IntegerLiteral*>(e);
        auto intType = intLit->intType();
        // If it's an untyped integer literal, then just create a new literal of the desired size.
        if (auto dstInt = dyn_cast_or_null<IntegerType>(dstType)) {
          if (intType->isImplicitlySized()) {
            // intLit->type = dstInt;
            // return intLit;
            llvm::APInt intVal;
            if (intType->bits() > dstInt->bits()) {
              intVal = intLit->asAPInt().trunc(dstInt->bits());
            } else if (intType->bits() < dstInt->bits()) {
              if (intType->isUnsigned()) {
                intVal = intLit->asAPInt().zext(dstInt->bits());
              } else {
                intVal = intLit->asAPInt().sext(dstInt->bits());
              }
            }
            auto ni = new (*_alloc) IntegerLiteral(
              intLit->location,
              _alloc->copyOf(intVal),
              dstInt);
            return ni;
          }
        }

        return addCastIfNeeded(e, dstType);
      }

      case Expr::Kind::FLOAT_LITERAL: {
        assert(e->type);
        return addCastIfNeeded(e, dstType);
      }

      case Expr::Kind::CALL: {
        assert(e->type);
        auto callExpr = static_cast<ApplyFnOp*>(e);
        coerceExpr(callExpr->function, nullptr);
        if (auto fnRef = dyn_cast<DefnRef>(callExpr->function)) {
          auto fn = dyn_cast<FunctionDefn>(unwrapSpecialization(fnRef->defn));
          if (fn->intrinsic() != IntrinsicFn::NONE) {
            e = replaceIntrinsic(callExpr);
          }
        }
        // Deep coercion of arg trees should already have been handled.
        return addCastIfNeeded(e, dstType);
      }

      case Expr::Kind::MEMBER_NAME_REF: {
        // We no longer need the MemberNameRef, just what it resolved to.
        auto mref = cast<MemberNameRef>(e);
        assert(mref->refs != nullptr);
        coerceExpr(mref->refs, nullptr);
      }

      case Expr::Kind::VAR_REF: {
        // Reference to a variable name.
        auto dref = static_cast<DefnRef*>(e);
        auto vd = cast<ValueDefn>(dref->defn);
        coerceExpr(dref->stem, nullptr);
        dref->type = vd->type();
        return addCastIfNeeded(dref, dstType);
      }

      case Expr::Kind::FUNCTION_REF: {
        // Reference to a function name.
        auto dref = static_cast<DefnRef*>(e);
        ArrayRef<const Type*> typeArgs;
        Member* method = unwrapSpecialization(dref->defn, typeArgs);
        coerceExpr(dref->stem, nullptr);

        if (auto fd = dyn_cast<FunctionDefn>(method)) {
          ApplySpecialization transform(typeArgs);
          dref->type = transform.transform(fd->type());
        } else {
          assert(false && "Invalid function ref");
        }
        return dref;
      }

      case Expr::Kind::TYPE_REF: {
        auto dref = static_cast<DefnRef*>(e);
        if (auto spec = dyn_cast<SpecializedDefn>(dref->defn)) {
          dref->type = spec->type();
        } else if (auto td = dyn_cast<TypeDefn>(dref->defn)) {
          dref->type = td->type();
        }
        return addCastIfNeeded(dref, dstType);
      }

      case Expr::Kind::BLOCK: {
        auto blk = static_cast<BlockStmt*>(e);
        for (auto stmt : blk->stmts) {
          coerceExpr(stmt, nullptr);
        }
        if (blk->result) {
          blk->result = coerceExpr(blk->result, dstType);
          blk->type = blk->result->type;
        } else {
          blk->type = &VoidType::VOID;
        }
        return blk;
      }

      case Expr::Kind::LOCAL_VAR: {
        // Already done.
        return e;
      }

      case Expr::Kind::IF: {
        auto st = static_cast<IfStmt*>(e);
        st->test = coerceExpr(st->test, &BooleanType::BOOL);
        if (st->elseBlock != nullptr) {
          st->thenBlock = coerceExpr(st->thenBlock, dstType);
          st->elseBlock = coerceExpr(st->elseBlock, dstType);
          auto combined = combineTypes({ st->thenBlock->type, st->elseBlock->type });
          st->type = combined;
        } else {
          coerceExpr(st->thenBlock, dstType);
          auto combined = combineTypes({ st->thenBlock->type, &VoidType::VOID });
          st->thenBlock = addCastIfNeeded(st->thenBlock, combined);
          st->type = combined;
        }

        return st;
      }

      case Expr::Kind::WHILE: {
        auto st = static_cast<WhileStmt*>(e);
        st->test = coerceExpr(st->test, &BooleanType::BOOL);
        st->body = coerceExpr(st->test, nullptr);
        st->type = &VoidType::VOID;
        return st;
      }

      case Expr::Kind::RETURN: {
        auto ret = static_cast<UnaryOp*>(e);
        if (ret->arg && _subject) {
          ret->arg = coerceExpr(ret->arg, _functionReturnType);
        }
        ret->type = &Type::NO_RETURN; // Flow of control does not continue.
        return ret;
      }

  // def visitThrow(self, expr, cs):
  //   '''@type expr: spark.graph.graph.Throw
  //      @type cs: constraintsolver.ConstraintSolver'''
  //   if expr.hasArg():
  //     self.assignTypes(expr.getArg(), self.typeStore.getEssentialTypes()['throwable'].getType())
  //   return self.NO_RETURN

      case Expr::Kind::EQ:
      case Expr::Kind::NE: {
        auto op = static_cast<BinaryOp*>(e);
        coerceExpr(op->args[0], nullptr);
        coerceExpr(op->args[1], nullptr);
        op->type = &BooleanType::BOOL;
        return addCastIfNeeded(op, dstType);
      }

      case Expr::Kind::NOT: {
        auto op = static_cast<UnaryOp*>(e);
        // TODO: Implicit boolean conversion
        coerceExpr(op->arg, nullptr);
        op->type = &BooleanType::BOOL;
        return addCastIfNeeded(op, dstType);
      }

      case Expr::Kind::ASSIGN: {
        auto op = static_cast<BinaryOp*>(e);
        coerceExpr(op->args[0], nullptr);
        op->args[1] = coerceExpr(op->args[1], op->args[0]->type);
        op->type = &Type::NOT_EXPR;
        return op;
      }

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(e->kind);
        assert(false && "Invalid expression kind");
    }
  }

  Expr* ResolveTypesPass::replaceIntrinsic(ApplyFnOp* call) {
    auto fnRef = cast<DefnRef>(call->function);
    ArrayRef<const Type*> typeArgs;
    auto fd = cast<FunctionDefn>(unwrapSpecialization(fnRef->defn, typeArgs));
    switch (fd->intrinsic()) {
      case IntrinsicFn::EQ: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::EQ, call->location, call->args[0], call->args[1], &BooleanType::BOOL);
      }

      case IntrinsicFn::LE: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::LE, call->location, call->args[0], call->args[1], &BooleanType::BOOL);
      }

      case IntrinsicFn::LT: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::LT, call->location, call->args[0], call->args[1], &BooleanType::BOOL);
      }

      case IntrinsicFn::ADD: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::ADD, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::SUBTRACT: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::SUBTRACT, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::MULTIPLY: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::MULTIPLY, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::DIVIDE: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::DIVIDE, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::REMAINDER: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::REMAINDER, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::LSHIFT: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::LSHIFT, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::RSHIFT: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::RSHIFT, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::BIT_AND: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::BIT_AND, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::BIT_OR: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::BIT_OR, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      case IntrinsicFn::BIT_XOR: {
        return new (*_alloc) BinaryOp(
            Expr::Kind::BIT_XOR, call->location, call->args[0], call->args[1], typeArgs[0]);
      }

      default:
        return call;
    }
  }

  // Utilities

  Expr* ResolveTypesPass::booleanTest(Expr* expr) {
    auto type = assignTypes(expr, nullptr);
    if (type->kind == Type::Kind::BOOLEAN) {
      return expr;
    } else if (type->kind == Type::Kind::INTEGER) {
      auto intTy = cast<IntegerType>(type);
      llvm::APInt(intTy->bits(), 0, false);
      return new BinaryOp(Expr::Kind::NE, expr, new IntegerLiteral(0, intTy));
    } else {
      // TODO: Check for collections
      diag.error(expr) << "Cannot coerce value to boolean type.";
      return expr;
    }
  }

  bool ResolveTypesPass::lookupADLName(MemberListExpr* m, ArrayRef<Type*> argTypes) {
    assert(false);
  }

  Expr* ResolveTypesPass::resolveMemberNameRef(MemberNameRef* mref) {
    auto stemType = assignTypes(mref->stem, nullptr);
    if (Type::isError(stemType)) {
      return &Expr::ERROR;
    }

    MemberNameLookup lookup(_cu.spec());
    MemberLookupResult members;
    lookup.lookup(mref->name, stemType, members, MemberNameLookup::INSTANCE_MEMBERS);

    if (members.size() > 0) {
      mref->refs = createNameRef(*_alloc, mref->location, members,
          cast_or_null<Defn>(_scope->subject()),
          mref->stem, false, false);
    } else {
      diag.error(mref) << "Member `" << mref->name << "` not found.";
      ClosestName closest(mref->name);
      lookup.forAllNames(stemType, std::ref(closest));
      if (!closest.bestMatch.empty()) {
        diag.info() << "Did you mean '" << closest.bestMatch << "'?";
      }
      mref->refs = &Expr::ERROR;
    }

    return mref->refs;
  }

  Expr* ResolveTypesPass::addCastIfNeeded(Expr* expr, const Type* ty) {
    if (expr == nullptr || ty == nullptr || Expr::isError(expr) || Type::isError(ty)) {
      return expr;
    }
    TypeCasts casting(*_alloc);
    return casting.cast(ty, expr);
  }

  /** Given a set of alternative types, determine the smallest set of types that is assigable
      from all the input types. If it's a single type, return it, otherwise return a union of
      the results. */
  const Type* ResolveTypesPass::combineTypes(ArrayRef<const Type*> types) {
    SmallVector<const Type*, 8> results;

    std::function<void(const Type*)> addtype;
    addtype = [&results, &addtype](const Type* t) {
      // Don't include error types or types that don't yield a value.
      if (Type::isError(t) || t->kind == Type::Kind::NEVER) {
        return;
      }

      if (auto ut = dyn_cast<UnionType>(t)) {
        for (auto el : ut->members) {
          addtype(el);
        }
      } else {
        SmallVector<const Type*, 8> preserved;
        auto addNew = true;
        for (auto ot : results) {
          if (isAssignable(ot, t).rank > ConversionRank::INEXACT) {
            // There's already a type in the list that is more general.
            addNew = false;
            preserved.push_back(ot);
          } else if (isAssignable(t, ot).rank <= ConversionRank::INEXACT) {
            // Preserve the old type, since the new type is not more general.
            preserved.push_back(ot);
          }
        }

        if (addNew) {
          preserved.push_back(t);
        }
        results.swap(preserved);
      }
    };

    for (auto t : types) {
      addtype(t);
    }

    if (results.size() == 0) {
      return &Type::NO_RETURN;
    } else if (results.size() == 1) {
      return results.front();
    } else {
      return _cu.types().createUnionType(results);
    }
  }

  const Type* ResolveTypesPass::chooseIntegerType(const Type* ty) {
    assert(ty);
    // if isinstance(expr, graph.Pack):
    //   assert isinstance(ty, graph.TupleType)
    //   assert len(expr.getArgs()) == len(ty.getMembers())
    //   memberTypes = []
    //   for arg, argType in zip(expr.getArgs(), ty.getMembers()):
    //     memberTypes.append(self.chooseIntegerType(arg, argType))
    //   return self.typeStore.newTupleType(memberTypes)
    if (ty->kind != Type::Kind::INTEGER) {
      return ty;
    }
    auto intTy = static_cast<const IntegerType*>(ty);
    if (intTy->isImplicitlySized()) {
      auto bits = intTy->isUnsigned() ? intTy->bits() - 1 : intTy->bits();
      if (intTy->isUnsigned()) {
        return bits <= 32 ? &IntegerType::U32 : &IntegerType::U64;
      } else {
        return bits <= 32 ? &IntegerType::I32 : &IntegerType::I64;
      }
    }
    return ty;
  }
}
