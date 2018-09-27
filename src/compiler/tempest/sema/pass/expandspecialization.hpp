#ifndef TEMPEST_SEMA_PASS_EXPANDSPECIALIZATION_HPP
#define TEMPEST_SEMA_PASS_EXPANDSPECIALIZATION_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

namespace tempest::gen {
  class FunctionSym;
  class ClassDescriptorSym;
  class InterfaceDescriptorSym;
  class GlobalVarSym;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using namespace tempest::sema::graph;

  /** Helper which can do eager type resolution. */
  class ExpandSpecializationPass {
  public:
    ExpandSpecializationPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

    // Output symbols

    void visitFunctionSym(gen::FunctionSym* fsym);
    void visitClassDescriptorSym(gen::ClassDescriptorSym* cls);
    void visitInterfaceDescriptorSym(gen::InterfaceDescriptorSym* ifc);
    void visitGlobalVarSym(gen::GlobalVarSym* gvar);

    // Defns

    Defn* expandDefn(Defn* d, Env& env);
    bool expandList(llvm::SmallVectorImpl<Defn*>& out, DefnArray members, Env& env);
    TypeDefn* expandTypeDefn(TypeDefn* td, Env& env);
    FunctionDefn* expandFunctionDefn(FunctionDefn* fd, Env& env);
    ValueDefn* expandValueDefn(ValueDefn* vd, Env& env);

    // Exprs

    void visitExpr(Expr* expr, Env& env);
    bool expandExprList(llvm::SmallVectorImpl<Expr*>& out, ArrayRef<Expr*> exprs, Env& env);
    Expr* expandExpr(Expr* expr, Env& env);

    // Types

    bool expandTypeList(llvm::SmallVectorImpl<Type*>& out, ArrayRef<Type*> exprs, Env& env);
    Type* expandType(Type* expr, Env& env);

  protected:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    size_t _symbolsProcessed = 0;
    Module* _module = nullptr;

    void addSpecialization(SpecializedDefn* sp);
  };
}

#endif
