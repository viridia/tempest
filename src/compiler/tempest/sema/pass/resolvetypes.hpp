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
  class BlockStmt;
  class LocalVarStmt;
  class DefnRef;
  class MemberListExpr;
}

namespace tempest::sema::infer {
  class ConstraintSolver;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using tempest::sema::infer::ConstraintSolver;
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

    // Inference

    /** Solve type inference for this expression subtree. Note that if there are subtrees within
        this tree that don't contribute to the result type (like statements in a block) then those
        will be solved separately. */
    Type* assignTypes(Expr* e, const Type* dstType, bool downCast = false);

  protected:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    llvm::BumpPtrAllocator* _alloc = nullptr;

    Type* visitBlock(BlockStmt* expr, ConstraintSolver& cs);
    Type* visitLocalVar(LocalVarStmt* expr, ConstraintSolver& cs);
    Type* visitCall(ApplyFnOp* expr, ConstraintSolver& cs);
    Type* visitCallName(
        Expr* callExpr, Expr* fn, const llvm::ArrayRef<Expr*>& args, ConstraintSolver& cs);
    Type* addCallSite(
        Expr* callExpr,
        Expr* fn,
        const llvm::ArrayRef<Member*>& methodList,
        const llvm::ArrayRef<Expr*>& args,
        const llvm::ArrayRef<Type*>& argTypes,
        ConstraintSolver& cs);
    Type* visitVarName(DefnRef* expr, ConstraintSolver& cs);
    Type* visitFunctionName(DefnRef* expr, ConstraintSolver& cs);
    Type* visitTypeName(DefnRef* expr, ConstraintSolver& cs);

    Type* doTypeInference(Expr* expr, Type* exprType, ConstraintSolver& cs);
    Type* chooseIntegerType(Expr* expr, Type* ty);

    /** Make a copy of this array within the current alloc. */
    template<class T> llvm::ArrayRef<T> copyOf(llvm::ArrayRef<T> array) {
      auto data = static_cast<T*>(_alloc->Allocate(array.size(), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    template<class T> llvm::ArrayRef<T> copyOf(const llvm::SmallVectorImpl<T>& array) {
      auto data = static_cast<T*>(_alloc->Allocate(array.size(), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }
  };
}

#endif