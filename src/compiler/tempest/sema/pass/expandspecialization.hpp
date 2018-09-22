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

    // Defns

    void visitDefn(Defn* td, Env& env);
    void visitList(DefnArray members, Env& env);
    void visitTypeDefn(TypeDefn* td, Env& env);
    void visitFunctionDefn(FunctionDefn* fd, Env& env);
    void visitValueDefn(ValueDefn* vd, Env& env);
    void visitAttributes(Defn* defn, Env& env);

    // Exprs

    void visitExpr(Expr* expr, Env& env);

  protected:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    size_t _specializationsProcessed = 0;
    Module* _module = nullptr;

    void addSpecialization(SpecializedDefn* sp);
  };
}

#endif
