#ifndef TEMPEST_SEMA_PASS_RESOLVETYPES_HPP
#define TEMPEST_SEMA_PASS_RESOLVETYPES_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

namespace tempest::sema::graph {
  class ApplyFnOp;
  class BinaryOp;
  class BlockStmt;
  class LocalVarStmt;
  class Member;
  class DefnRef;
  class MemberNameRef;
  class IfStmt;
  class MemberListExpr;
  class WhileStmt;
}

namespace tempest::sema::infer {
  class ConstraintSolver;
  class CallCandidate;
  class CallSite;
  class SolutionTransform;
}

namespace tempest::sema::names {
  struct LookupScope;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Module;
  using tempest::sema::infer::CallCandidate;
  using tempest::sema::infer::CallSite;
  using tempest::sema::infer::ConstraintSolver;
  using tempest::sema::infer::SolutionTransform;
  using namespace tempest::sema::graph;

  /** Helper which can do eager type resolution. */
  class ResolveTypesPass {
  public:
    ResolveTypesPass(CompilationUnit& cu) : _cu(cu) {}

    /** Eagerly resolve types for a definition. */
    static bool resolve(Defn* defn);

    void run();

    /** Process a single module. */
    void process(Module* mod);

    /** Connect to module allocator. */
    void begin(Module* mod);

    // Defns

    void visitDefn(Defn* td);
    void visitList(DefnArray members);
    void visitTypeDefn(TypeDefn* td);
    void visitCompositeDefn(TypeDefn* td);
    void visitEnumDefn(TypeDefn* td);
    void visitFunctionDefn(FunctionDefn* fd);
    void visitValueDefn(ValueDefn* vd);
    void visitAttributes(Defn* defn);

    // Exprs

    const Type* visitExpr(Expr* expr, ConstraintSolver& cs);
    void visitExprArray(
        llvm::SmallVectorImpl<const Type*>& types,
        const llvm::ArrayRef<Expr*>& exprs,
        ConstraintSolver& cs);

    Expr* coerceExpr(Expr* expr, const Type* dstType);

    // Inference

    /** Solve type inference for this expression subtree. Note that if there are subtrees within
        this tree that don't contribute to the result type (like statements in a block) then those
        will be solved separately. */
    const Type* assignTypes(Expr* e, const Type* dstType, bool downCast = false);

  protected:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    tempest::support::BumpPtrAllocator* _alloc = nullptr;
    GenericDefn* _subject = nullptr;
    Module* _module = nullptr;
    const Type* _selfType = nullptr;
    std::vector<const Type*> _returnTypes;
    const Type* _functionReturnType;
    names::LookupScope* _scope = nullptr;

    const Type* visitBlock(BlockStmt* expr, ConstraintSolver& cs);
    const Type* visitLocalVar(LocalVarStmt* expr, ConstraintSolver& cs);
    const Type* visitIf(IfStmt* expr, ConstraintSolver& cs);
    const Type* visitWhile(WhileStmt* expr, ConstraintSolver& cs);
    const Type* visitAssign(BinaryOp* expr, ConstraintSolver& cs);
    const Type* visitCall(ApplyFnOp* expr, ConstraintSolver& cs);
    const Type* visitCallName(
        ApplyFnOp* callExpr, Expr* fn, const ArrayRef<Expr*>& args, ConstraintSolver& cs);
    void findConstructors(const ArrayRef<MemberAndStem>& types, MemberLookupResultRef& ctors);
    const Type* addCallSite(
        ApplyFnOp* callExpr,
        Expr* fn,
        const ArrayRef<MemberAndStem>& methodList,
        const ArrayRef<Expr*>& args,
        const ::ArrayRef<const Type*>& argTypes,
        ConstraintSolver& cs);
    const Type* visitVarName(DefnRef* expr, ConstraintSolver& cs);
    const Type* visitFunctionName(DefnRef* expr, ConstraintSolver& cs);
    const Type* visitTypeName(DefnRef* expr, ConstraintSolver& cs);

    Expr* booleanTest(Expr* expr);
    const Type* doTypeInference(Expr* expr, const Type* exprType, ConstraintSolver& cs);
    void applySolution(ConstraintSolver& cs, SolutionTransform& st);
    void updateCallSite(ConstraintSolver& cs, CallSite* site, SolutionTransform& st);
    void reorderCallingArgs(
        ConstraintSolver& cs, SolutionTransform& st, ApplyFnOp* callExpr,
        CallSite* site, CallCandidate* cc);
    bool lookupADLName(MemberListExpr* m, ArrayRef<Type*> argTypes);
    Expr* resolveMemberNameRef(MemberNameRef* mref);
    Expr* addCastIfNeeded(Expr* expr, const Type* ty);
    const Type* combineTypes(llvm::ArrayRef<const Type*> types);
    const Type* chooseIntegerType(const Type* ty);
    void ensureMutableLValue(Expr* expr);
    Expr* replaceIntrinsic(ApplyFnOp* expr);

    GenericDefn* setSubject(GenericDefn* subject) {
      auto prevSubject = _subject;
      _subject = subject;
      return prevSubject;
    }
  };
}

#endif
