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

    Type* visitExpr(Expr* expr, ConstraintSolver& cs);
    void visitExprArray(
        llvm::SmallVectorImpl<Type*>& types,
        const llvm::ArrayRef<Expr*>& exprs,
        ConstraintSolver& cs);

    Expr* coerceExpr(Expr* expr, Type* dstType);

    // Inference

    /** Solve type inference for this expression subtree. Note that if there are subtrees within
        this tree that don't contribute to the result type (like statements in a block) then those
        will be solved separately. */
    Type* assignTypes(Expr* e, const Type* dstType, bool downCast = false);

  protected:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    tempest::support::BumpPtrAllocator* _alloc = nullptr;
    GenericDefn* _subject = nullptr;
    Module* _module = nullptr;
    Type* _selfType = nullptr;
    std::vector<Type*> _returnTypes;
    const Type* _functionReturnType;
    names::LookupScope* _scope = nullptr;

    Type* visitBlock(BlockStmt* expr, ConstraintSolver& cs);
    Type* visitLocalVar(LocalVarStmt* expr, ConstraintSolver& cs);
    Type* visitIf(IfStmt* expr, ConstraintSolver& cs);
    Type* visitWhile(WhileStmt* expr, ConstraintSolver& cs);
    Type* visitAssign(BinaryOp* expr, ConstraintSolver& cs);
    Type* visitCall(ApplyFnOp* expr, ConstraintSolver& cs);
    Type* visitCallName(
        ApplyFnOp* callExpr, Expr* fn, const ArrayRef<Expr*>& args, ConstraintSolver& cs);
    void findConstructors(const ArrayRef<Member*>& types, NameLookupResultRef& ctors);
    Type* addCallSite(
        ApplyFnOp* callExpr,
        Expr* fn,
        const ArrayRef<Member*>& methodList,
        const ArrayRef<Expr*>& args,
        const ::ArrayRef<Type*>& argTypes,
        ConstraintSolver& cs);
    Type* visitVarName(DefnRef* expr, ConstraintSolver& cs);
    Type* visitFunctionName(DefnRef* expr, ConstraintSolver& cs);
    Type* visitTypeName(DefnRef* expr, ConstraintSolver& cs);

    Expr* booleanTest(Expr* expr);
    Type* doTypeInference(Expr* expr, Type* exprType, ConstraintSolver& cs);
    void applySolution(ConstraintSolver& cs, SolutionTransform& st);
    void updateCallSite(ConstraintSolver& cs, CallSite* site, SolutionTransform& st);
    void reorderCallingArgs(
        ConstraintSolver& cs, SolutionTransform& st, ApplyFnOp* callExpr,
        CallSite* site, CallCandidate* cc);
    bool lookupADLName(MemberListExpr* m, ArrayRef<Type*> argTypes);
    Expr* addCastIfNeeded(Expr* expr, Type* ty);
    Type* combineTypes(llvm::ArrayRef<Type*> types);
    Type* chooseIntegerType(Type* ty);

    GenericDefn* setSubject(GenericDefn* subject) {
      auto prevSubject = _subject;
      _subject = subject;
      return prevSubject;
    }
  };
}

#endif
