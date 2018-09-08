#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "tempest/sema/graph/env.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/infer/conditions.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/overload.hpp"
#include "tempest/sema/infer/paramassignments.hpp"
#include "tempest/sema/infer/solution.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "tempest/sema/transform/applyspec.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;
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
    visitList(mod->members());
  }

  void ResolveTypesPass::begin(Module* mod) {
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

      case Member::Kind::CONST_DEF:
      case Member::Kind::LET_DEF: {
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
    visitAttributes(td);
    if (td->type()->kind == Type::Kind::ALIAS) {
      // Should be already resolved.
    } else if (td->type()->kind == Type::Kind::ENUM) {
      visitEnumDefn(td);
    } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(td);
    }
    setSubject(prevSubject);
  }

  void ResolveTypesPass::visitCompositeDefn(TypeDefn* td) {
    assert(false && "Implement");
  }

  void ResolveTypesPass::visitEnumDefn(TypeDefn* td) {
    assert(false && "Implement");
  }

  void ResolveTypesPass::visitFunctionDefn(FunctionDefn* fd) {
    auto prevSubject = setSubject(fd);
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
      const Type* returnType = fd->type() ? fd->type()->returnType : nullptr;
      if (fd->isConstructor() && !fd->isStatic()) {
        returnType = &VoidType::VOID;
      }

      returnType = chooseIntegerType(fd->body(), assignTypes(fd->body(), returnType));
      if (returnType && !fd->type()) {
        SmallVector<Type*, 8> paramTypes;
        for (auto param : fd->params()) {
          paramTypes.push_back(param->type());
        }

        fd->setType(_cu.types().createFunctionType(returnType, paramTypes, fd->isVariadic()));
      }
    }
//     if func.hasBody() and not isinstance(func.getBody(), graph.Intrinsic):
//       if func.isConstructor() and not func.isStatic():
//         returnType = primitivetypes.VOID
//       elif func.getType().hasReturnType():
//         returnType = func.getType().getReturnType()
//       else:
//         returnType = None
//       assert func.getBody(), debug.format(func)
//       self.errorCount = self.errorReporter.errorCount
//       self.assignTypes(func.getBody(), returnType)
//       self.errorCount = self.errorReporter.errorCount
//     if func.isConstructor() and not func.isStatic():
//       assert func.hasSelfType(), debug.format(func, func.getType())
//     self.tempVarTypes = savedTempVars
//     self.nameLookup.setSubject(savedSubject)
    setSubject(prevSubject);
  }

  void ResolveTypesPass::visitValueDefn(ValueDefn* vd) {
    visitAttributes(vd);
    if (vd->init()) {
      auto initType = assignTypes(vd->init(), vd->type());
      if (!vd->type() && !Type::isError(initType)) {
        vd->setType(chooseIntegerType(vd->init(), initType));
      }
    }
  }

//   @accept(spark.graph.defn.Parameter)
//   def visitParameter(self, param):
//     '@type param: spark.graph.graph.Parameter'
//     self.visitAttributes(param)
//     if not param.hasType():
//       self.errorAt(
//           param.getLocation(), "Parameters must have an explicit type.")
//       param.setType(graph.ErrorType.defaultInstance())
//       return
//     self.membersVisited.add(param)
//     paramType = param.getType()
//     if graphtools.isVariadicParam(paramType):
//       paramType = self.typeStore.createArrayType(types.stripVariadic(paramType))
//       param.setInternalType(paramType)

//       # Check to see if the param type has the appropriate required methods.

//     if param.hasInit():
//       paramType = self.assignTypes(param.getInit(), paramType)
//       assert paramType, param.getName()

  void ResolveTypesPass::visitAttributes(Defn* defn) {
    for (auto attr : defn->attributes()) {
      assignTypes(attr, intrinsic::IntrinsicDefns::get()->objectClass.get()->type());
    }
  }

  // Type Inference

  Type* ResolveTypesPass::assignTypes(Expr* e, const Type* dstType, bool downCast) {
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
    //   if not cs.empty():
    //     return self.doTypeInference(expr, exprType, cs)
    //   return typesubstitution.UnbindTypeParams().traverseType(exprType)
    // except AssertionError:
    //   self.errorAt(expr, 'Assertion error')
    //   raise
    return exprType;
    // assert(false && "Implement");
  }

  // Expressions

  Type* ResolveTypesPass::visitExpr(Expr* e, ConstraintSolver& cs) {
    switch (e->kind) {
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

      case Expr::Kind::VAR_REF:
        return visitVarName(static_cast<DefnRef*>(e), cs);

      case Expr::Kind::FUNCTION_REF:
        return visitFunctionName(static_cast<DefnRef*>(e), cs);

      case Expr::Kind::TYPE_REF:
        return visitTypeName(static_cast<DefnRef*>(e), cs);

      case Expr::Kind::BLOCK:
        return visitBlock(static_cast<BlockStmt*>(e), cs);

      case Expr::Kind::LOCAL_VAR:
        return visitLocalVar(static_cast<LocalVarStmt*>(e), cs);

      case Expr::Kind::RETURN: {
        auto ret = static_cast<UnaryOp*>(e);
        auto fd = dyn_cast<FunctionDefn>(_subject);
        if (ret->arg) {
          ret->type = assignTypes(ret->arg, fd ? fd->type()->returnType : nullptr);
        } else {
          ret->type = &VoidType::VOID;
        }
        return &Type::NO_RETURN;
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
      llvm::SmallVectorImpl<Type*>& types,
      const llvm::ArrayRef<Expr*>& exprs,
      ConstraintSolver& cs) {
    for (auto expr : exprs) {
      types.push_back(visitExpr(expr, cs));
    }
  }

  Type* ResolveTypesPass::visitBlock(BlockStmt* blk, ConstraintSolver& cs) {
    bool earlyExit = false;
    auto errorCount = diag.errorCount();
    for (auto st : blk->stmts) {
      if (earlyExit) {
        diag.error(st) << "Unreachable code.";
      }
      auto sTy = assignTypes(st, nullptr);
      if (diag.errorCount() != errorCount) {
        return &Type::ERROR;
      }
      if (sTy->kind == Type::Kind::NEVER) {
        earlyExit = true;
      }
    }

    if (blk->result) {
      if (earlyExit) {
        diag.error(blk->result) << "Unreachable code.";
      }
      return assignTypes(blk->result, nullptr);
    }

    return earlyExit ? &Type::NO_RETURN : &VoidType::VOID;
  }

  Type* ResolveTypesPass::visitLocalVar(LocalVarStmt* expr, ConstraintSolver& cs) {
    auto vd = expr->defn;
    if (vd->init()) {
      auto initType = assignTypes(vd->init(), vd->type());
      if (!vd->type() && !Type::isError(initType)) {
        vd->setType(chooseIntegerType(vd->init(), initType));
      }
    } else if (!vd->type()) {
      diag.error(expr) << "Local definition has no type and no initializer.";
    }

    return &Type::NOT_EXPR;
  }

  Type* ResolveTypesPass::visitCall(ApplyFnOp* expr, ConstraintSolver& cs) {
    switch (expr->function->kind) {
      case Expr::Kind::FUNCTION_REF_OVERLOAD: {
        return visitCallName(expr, expr->function, expr->args, cs);
        break;
      }
      case Expr::Kind::TYPE_REF_OVERLOAD: {
        // Construct a type.
        assert(false && "Implement");
        break;
      }
      case Expr::Kind::SUPER: {
        assert(false && "Implement");
        break;
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

  Type* ResolveTypesPass::visitCallName(
      ApplyFnOp* callExpr, Expr* fn, const ArrayRef<Expr*>& args, ConstraintSolver& cs) {

    // Gather the types of all arguments.
    SmallVector<Type*, 8> argTypes;
    visitExprArray(argTypes, args, cs);
    for (auto t : argTypes) {
      if (Type::isError(t)) {
        return &Type::ERROR;
      }
    }
    for (size_t i = 0; i < argTypes.size(); i += 1) {
      if (args[i]->kind == Expr::Kind::INTEGER_LITERAL) {
        argTypes[i] = chooseIntegerType(args[i], argTypes[i]);
      }
    }

//     cs.renamer.renamerChecker.traverseTypeList(argTypes)

    if (fn->kind == Expr::Kind::FUNCTION_REF_OVERLOAD) {
      auto fnRef = static_cast<MemberListExpr*>(fn);
      return addCallSite(callExpr, fn, fnRef->members, args, argTypes, cs);
    }

//     if isinstance(func, graph.ExplicitSpecialize):
//       func = self.lookupSpecializedCallable(func, argTypes, cs)
//       if self.isErrorExpr(func):
//         return graph.ErrorType.defaultInstance()
//       callExpr.getArgs()[0] = func
//     elif func.getListType() == graph.MemberListType.INCOMPLETE:
//       func = self.lookupCallable(func, argTypes, cs)
//       if self.isErrorExpr(func):
//         return graph.ErrorType.defaultInstance()

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
//     elif listType == graph.MemberListType.TYPE:
//       func = self.findConstructors(func, cs)
//       if self.isErrorExpr(func):
//         return graph.ErrorType.defaultInstance()
//       callExpr.getMutableArgs()[0] = func
//     else:
//       assert listType == graph.MemberListType.FUNCTION


    assert(false && "Implement");
  }

  Type* ResolveTypesPass::addCallSite(
      ApplyFnOp* callExpr,
      Expr* fn,
      const ArrayRef<Member*>& methodList,
      const ArrayRef<Expr*>& args,
      const ArrayRef<Type*>& argTypes,
      ConstraintSolver& cs) {
    auto site = new CallSite(callExpr->location, callExpr, args, argTypes);
    cs.addSite(site);

    // The return type will depend on which overload type gets chosen.
    SmallVector<ContingentType::Entry, 8> returnTypes;

    // For each possible function that could have been called.
    for (auto method : methodList) {
      auto cc = site->addCandidate(method);

      // Collect explicit type arguments
      std::unordered_map<TypeParameter*, const Type*> explicitTypeArgs;
      if (method->kind == Member::Kind::SPECIALIZED) {
        auto sp = static_cast<SpecializedDefn*>(method);
        for (size_t i = 0; i < sp->typeParams().size(); i += 1) {
          auto arg = sp->typeArgs()[i];
          auto param = sp->typeParams()[i];
          if (arg) {
            explicitTypeArgs[param] = arg;
          }
        }
        method = sp->generic();
      }

      // Make sure function type is resolved.
      auto function = cast<FunctionDefn>(method);
      auto params = function->params();
      if (!function->type()) {
        if (!resolve(function)) {
          return &Type::ERROR;
        }
      }
      auto returnType = function->type()->returnType;

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
            cs.addBindingConstraint(
                site->location, typeArg, transform.transform(subtype),
                TypeRelation::SUBTYPE, cc);
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
      return const_cast<Type *>(commonReturnType); // Ugh
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

//   def findConstructors(self, base, cs):
//     # Look for a constructor
//     mlist = graph.MemberList().setLocation(base.getLocation()).setName('new')
//     mlist.setBase(base)
//     if not self.lookupMembersByName(mlist, cs, self.nameLookup.constructorNotFound):
//       return graph.ErrorType.defaultInstance()
//     assert len(mlist.getMembers()) > 0
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

  Type* ResolveTypesPass::visitVarName(DefnRef* expr, ConstraintSolver& cs) {
    auto vd = cast<ValueDefn>(expr->defn);
    if (!vd->type()) {
      assert(false && "Do eager type resolution");
    }
    return vd->type();
  }

  Type* ResolveTypesPass::visitFunctionName(DefnRef* expr, ConstraintSolver& cs) {
    assert(false && "Implement");
  }

  Type* ResolveTypesPass::visitTypeName(DefnRef* expr, ConstraintSolver& cs) {
    assert(false && "Implement");
  }

  // Type Inference

  Type* ResolveTypesPass::doTypeInference(Expr* expr, Type* exprType, ConstraintSolver& cs) {

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
//       debug.tracing = False
//       if cs.checkSolution():
//         solutionEnv = cs.createSolutionEnv()
    applySolution(cs);
//         self.applySolution(cs, expr, solutionEnv)
//         if exprType:
//           finalExprType = typesubstitution.ApplyEnv(solutionEnv).traverseType(exprType)
//           finalExprType = callsite.CollapseAmbiguousTypes(
//               self.typeStore).traverseType(finalExprType)
//           renamer.EnsureAllTypeVarsAreNormalized().traverseType(finalExprType)
//           if self.nameLookup.getSubject().getName() in TRACE_SUBJECTS:
//             cs.dump()
//             self.infoAt(expr, 'Inference for expression')
//             self.infoFmt('solution = {0}', debug.format(solutionEnv))
//           return finalExprType
    return const_cast<Type*>(transform.transform(exprType));
//       else:
//         debug.write(
//             "--------------------------------------------------------------------------------")
//         cs.dump()
//         return graph.ErrorType.defaultInstance()
//     except AssertionError as e:
//       self.info(
//           "--------------------------------------------------------------------------------")
//       self.errorAt(expr, 'Assertion error')
//       cs.dump()
//       if solutionEnv:
//         debug.write('Solution Env:', solutionEnv)
//       raise Exception() from e
    // assert(false && "Implement");
  }

  void ResolveTypesPass::applySolution(ConstraintSolver& cs) {
//   def applySolution(self, cs, root, solutionEnv):
//     nonContextualTypeVars = cs.renamer.getNonContextualTypeVars()

//     if solutionEnv:
//       solutionTransform = typesubstitution.ApplyEnv(solutionEnv)

    for (auto site : cs.sites()) {

//     for site in cs.sites:
//       if not isinstance(site, callsite.CallSite):
//         continue
//       candidate = site.getSingleViableCandidate()
//       assert candidate is not None
//       assert isinstance(candidate.returnType, graph.Type)
//       method = candidate.member
//       methodEnv = environ.EMPTY
//       if isinstance(method, graph.InstantiatedMember):
//         methodEnv = method.getEnv()
//         method = method.getBase()
//       needsBaseExpr = method and not defns.isStaticallyCallable(method)
//       candidate.methodEnv = methodEnv
//       if candidate.typeVars:
// #         transformMap = {}
//         transformMap = dict(solutionTransform.traverseEnv(candidate.typeVars).items())
// #         for tv, inferredVar in candidate.typeVars.items():
// #           if inferredVar in solutionEnv:
// #             mappedVar = solutionTransform.traverseType(inferredVar)
// #             if tv == mappedVar and isinstance(mappedVar, graph.TypeVar):
// #               debug.write(root)
// #               debug.write(tv, '-->', inferredVar, '-->', mappedVar, typeVars=candidate.typeVars, solutionEnv=solutionEnv)
// # #               continue
// #             transformMap[tv] = mappedVar
// # #           debug.write('mapping', inferredVar, 'to', transformMap[tv])

//         # Add in other solution entries that are not associated with any candidate.
//         for inferredVar in nonContextualTypeVars:
//           explicitVar = inferredVar.getParam().getTypeVar()
//           assert explicitVar is not inferredVar
//           if inferredVar in solutionEnv:
//             value = solutionEnv[inferredVar]
//             if explicitVar not in transformMap and explicitVar is not value:
//               transformMap[explicitVar] = value

//         for tvar, value in methodEnv.items():
//           if tvar not in transformMap:
// #             debug.write('Additional methodEnv mapping', tvar, ':', value)
//             transformMap[tvar] = value

//         methodEnv = environ.Env(transformMap)
//         candidate.methodEnv = methodEnv
//       transform = None
//       replaceMethods = None
//       if len(candidate.requiredMethodMap) > 0:
//         replaceMethods = methodsubstitution.MethodSubsitution()
//         replaceMethods.methodMap.update(candidate.requiredMethodMap)
//       if len(methodEnv) > 0:
// #         debug.write('solution method', method, methodEnv)
//         assert not isinstance(method, graph.InstantiatedMember), 'optimize'
//         method = graph.InstantiatedMember().setName(method.getName()).setEnv(methodEnv).setBase(method)
//         transform = typesubstitution.ApplyEnv(methodEnv)
// #         solutionTransform = typesubstitution.ApplyEnv(solutionEnv)
//         candidate.returnType = solutionTransform.traverseType(candidate.returnType)
// #         candidate.paramList = transform.traverseParamList(candidate.paramList)
//         candidate.paramTypes = solutionTransform.traverseTypeList(candidate.paramTypes)
//         renamer.EnsureAllTypeVarsAreNormalized().traverseTypeList(candidate.paramTypes)
      if (site->kind == OverloadKind::CALL) {
        CallSite* c = static_cast<CallSite*>(site);
        updateCallSite(cs, c);
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

  void ResolveTypesPass::updateCallSite(ConstraintSolver& cs, CallSite* site) {
    auto candidate = static_cast<CallCandidate*>(site->singularCandidate());
    auto method = unwrapSpecialization(candidate->method);
    auto callExpr = static_cast<ApplyFnOp*>(site->callExpr);

    // Patch the call expression with a new callable
    if (callExpr->function->kind == Expr::Kind::FUNCTION_REF_OVERLOAD) {
      auto fnRef = static_cast<MemberListExpr*>(callExpr->function);
      callExpr->function =
          new (*_alloc) DefnRef(Expr::Kind::FUNCTION_REF, site->location, method, fnRef->stem);
    } else {
      assert(false && "Implement other callable types");
    }

    reorderCallingArgs(cs, callExpr, site, candidate);
  }
//       self.reorderCallingArgs(site.callExpr, candidate, transform, replaceMethods)

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

//     # Check to make sure that the result is concrete
//     for site in cs.sites:
//       candidate = site.getSingleViableCandidate()
//       if isinstance(site, callsite.CallSite):
//         method = candidate.member
//         if method:
//           mlist = site.callExpr.getArgs()[0]
//           if mlist.hasBase():
//             if self.nameLookup.subject.getName() == 'copyFrom' or True:
//               subjectEnv = environ.Env({tp.getTypeVar(): primitivetypes.VOID for tp in cs.outerParams})
//               try:
//                 graphtools.ASSERT_CONCRETE_VISITOR.visit(mlist.getBase(), (subjectEnv,))
//               except:
//                 debug.write('  in subject:', self.nameLookup.subject)
//                 debug.write('  with method env:', candidate.methodEnv)
//                 debug.write('  with subject vars:', set(subjectEnv.keys()))
//                 debug.write('  with outer params:', cs.outerParams)
//                 debug.write('  with non-contextual:', nonContextualTypeVars)
//                 raise
//             renamer.EnsureAllTypeVarsAreNormalized().traverseExpr(mlist.getBase())
//       elif isinstance(site, callsite.TypeChoicePoint):
//         pass
// #         t = candidate.member
// #         debug.write('t', t)

// #     typesubstitution.ApplyEnv(solutionEnv).traverseExpr(root)
//     renamer.EnsureAllTypeVarsAreNormalized().traverseExpr(root)

  void ResolveTypesPass::reorderCallingArgs(
      ConstraintSolver& cs, ApplyFnOp* callExpr, CallSite* site, CallCandidate* cc) {
    size_t argIndex = 0;
    SmallVector<Expr*, 8> argList;
    SmallVector<Expr*, 8> varArgList;
    argList.resize(cc->params.size(), nullptr);

    // Assign explicit arguments
    for (auto arg : site->argList) {
      size_t paramIndex = cc->paramAssignments[argIndex++];
      if (cc->isVariadic && paramIndex == cc->paramTypes.size() - 1) {
        varArgList.push_back(arg);
      } else {
        argList[paramIndex] = arg;
      }
    }

    // Handle rest args
    if (cc->isVariadic) {
      MultiOp* restArgs = new (*_alloc) MultiOp(
          Expr::Kind::REST_ARGS, callExpr->location, _alloc->copyOf(varArgList),
          const_cast<Type*>(cc->paramTypes.back()));
      argList[cc->paramTypes.size() - 1] = restArgs;
    }

    // Now, fill in defaults
    for (size_t i = 0; i < cc->params.size(); i += 1) {
      if (!argList[i]) {
        auto param = cc->params[i];
        if (param->init()) {
          // TODO: Transform
          argList[i] = param->init();
        }
      }
    }

    callExpr->args = _alloc->copyOf(argList);
  }

  // Utilities

  Type* ResolveTypesPass::chooseIntegerType(Expr* expr, Type* ty) {
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
    if (expr->kind == Expr::Kind::INTEGER_LITERAL) {
      auto intLit = static_cast<IntegerLiteral*>(expr);
      auto bits = intLit->isUnsigned
          ? intLit->value().getActiveBits() : intLit->value().getMinSignedBits();
      if (intLit->isUnsigned) {
        return bits <= 32 ? &IntegerType::U32 : &IntegerType::U64;
      } else {
        return bits <= 32 ? &IntegerType::I32 : &IntegerType::I64;
      }
    }
    return ty;
  }
}
