#include "tempest/error/diagnostics.hpp"
#include "tempest/intrinsic/defns.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "llvm/Support/Casting.h"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;

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

  void ResolveTypesPass::visit(Defn* defn) {
    if (defn->isResolved()) {
      return;
    }
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
  }

  void ResolveTypesPass::begin(Module* mod) {
    _alloc = &mod->semaAlloc();
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
    visitAttributes(td);
    if (td->type()->kind == Type::Kind::ALIAS) {
      // Should be already resolved.
    } else if (td->type()->kind == Type::Kind::ENUM) {
      visitEnumDefn(td);
    } else if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(td);
    }
  }

  void ResolveTypesPass::visitCompositeDefn(TypeDefn* td) {
    assert(false && "Implement");
  }

  void ResolveTypesPass::visitEnumDefn(TypeDefn* td) {
    assert(false && "Implement");
  }

  void ResolveTypesPass::visitFunctionDefn(FunctionDefn* fd) {
//   @accept(spark.graph.defn.Function)
//   def visitFunction(self, func):
//     '@type func: spark.graph.graph.Function'
//     if func in self.membersVisited:
//       return
//     if not func.isMutable():
//       self.membersVisited.add(func)
//       return
//     savedSubject = self.nameLookup.setSubject(func)
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

//   @accept(spark.graph.defn.ValueDefn)
//   def visitValueDefn(self, vdef):
//     '@type vdef: spark.graph.graph.ValueDefn'
//     if vdef in self.membersVisited:
//       return
//     if vdef.hasDefinedIn():
//       savedSubject = self.nameLookup.setSubject(vdef.getDefinedIn())
//     self.visitAttributes(vdef)
//     if vdef.hasDefinedIn():
//       self.nameLookup.setSubject(savedSubject)

//     initType = None
//     if not vdef.hasType() and not vdef.hasInit():
//       assert not vdef.hasAstType()
//       assert not vdef.hasAstInit()
//       self.errorAt(vdef, "Variable must either have an explicit type or an initializer.")
//       vdef.setType(graph.ErrorType.defaultInstance())
//     elif vdef.hasType():
//       initType = vdef.getType()
//       self.membersVisited.add(vdef)
// #     print(vdef.getName(), vdef.hasType(), vdef.hasInit())
//     if vdef.hasInit():
//       if vdef.hasDefinedIn():
//         savedSubject = self.nameLookup.setSubject(vdef)
//       initType = self.assignTypes(vdef.getInit(), initType)
//       if vdef.hasDefinedIn():
//         self.nameLookup.setSubject(savedSubject)
//       assert initType, (vdef.getName(), type(vdef.getInit()))
//       if not vdef.hasType():
//         initType = self.chooseIntegerType(vdef.getInit(), initType)
//     if not vdef.hasType() and initType:
//       vdef.setType(initType)
// #     vdef.getType().freeze()
//     self.membersVisited.add(vdef)

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

//   @accept(spark.graph.defn.Property)
//   def visitProperty(self, prop):
//     '@type prop: spark.graph.graph.Property'
//     if prop in self.membersVisited:
//       return
//     self.visitAttributes(prop)
//     for param in prop.getParams():
//       self.visit(param)
//     self.membersVisited.add(prop)
//     self.visitList(prop.getMembers())

//   def visitAttributes(self, m):
//     '@type var: spark.graph.graph.Defn'
//     if len(m.getAttributes()) > 0:
//       objectType = self.typeStore.getEssentialTypes()['object'].getType()
//       for attr in m.getAttributes():
//         self.assignTypes(attr, objectType)

  void ResolveTypesPass::visitAttributes(Defn* defn) {
    for (auto attr : defn->attributes()) {
      assignTypes(attr, intrinsic::IntrinsicDefns::get()->objectClass.get()->type());
    }
  }

  Type* ResolveTypesPass::assignTypes(Expr* e, const Type* dstType, bool downCast) {
    Location location = e->location;
    ConstraintSolver cs(location);
    // cs = constraintsolver.ConstraintSolver(
    //     location, self, self.typeStore, self.nameLookup.getSubject())
    // try:
    auto errorCount = diag.errorCount();
    //   errorCount = self.errorReporter.getErrorCount()
    //   if self.nameLookup.getSubject().getName() in TRACE_SUBJECTS:
    //     debug.write('tracing:', self.nameLookup.getSubject().getName())
    //     debug.tracing = True
    auto exprType = visitExpr(e, cs);
    //   exprType = self.traverseExpr(expr, cs)
    if (errorCount != diag.errorCount()) {
      return &Type::ERROR;
    }

    diag.debug() << "Expression type: " << exprType;
    //   assert exprType.typeId() != graph.TypeKind.BASE, debug.format(expr)
    //   cs.renamer.renamerChecker.traverseType(exprType)
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
    //     cs.renamer.renamerChecker.traverseType(dstType)
    //     cs.addAssignment(None, dstType, exprType)
    //   if not cs.empty():
    //     return self.doTypeInference(expr, exprType, cs)
    //   return typesubstitution.UnbindTypeParams().traverseType(exprType)
    // except AssertionError:
    //   self.errorAt(expr, 'Assertion error')
    //   raise
    return exprType;
    // assert(false && "Implement");
  }

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

      default:
        diag.debug() << "Invalid expression kind: " << Expr::KindName(e->kind);
        assert(false && "Invalid expression kind");
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
    }

    if (!vd->type()) {
      diag.error(expr) << "Local definition has no type and no initializer.";
    }

    return &Type::NOT_EXPR;
  }

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
