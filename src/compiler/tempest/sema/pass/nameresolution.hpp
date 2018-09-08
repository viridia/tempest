#ifndef TEMPEST_SEMA_PASS_NAMERESOLUTION_HPP
#define TEMPEST_SEMA_PASS_NAMERESOLUTION_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

namespace tempest::ast {
  class Defn;
  class TypeDefn;
  class FunctionDefn;
  class Oper;
  class ValueDefn;
}

namespace tempest::sema::graph {
  class Defn;
  class Expr;
}

namespace tempest::sema::names {
  struct LookupScope;
}

namespace tempest::sema::pass {
  using tempest::compiler::CompilationUnit;
  using tempest::sema::graph::Module;
  using tempest::sema::names::LookupScope;
  using namespace tempest::sema::graph;

  /** Pass which resolves references to names. */
  class NameResolutionPass {
  public:
    NameResolutionPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

    // Module

    void resolveImports(Module* mod);

    // Defns

    void visitList(LookupScope* scope, DefnArray members);
    void visitTypeDefn(LookupScope* scope, TypeDefn* td);
    void visitCompositeDefn(LookupScope* scope, TypeDefn* td);
    void visitEnumDefn(LookupScope* scope, TypeDefn* td);
    void visitFunctionDefn(LookupScope* scope, FunctionDefn* fd);
    void visitValueDefn(LookupScope* scope, ValueDefn* vd);

    // Exprs

    Expr* visitExpr(LookupScope* scope, const ast::Node* in);
    Expr* visitSpecialize(LookupScope* scope, const ast::Oper* node);

    // Types

    Type* resolveType(LookupScope* scope, const ast::Node* ast);

    // Utility

    bool resolveDefnName(LookupScope* scope, const ast::Node* node, NameLookupResultRef& result);
    bool resolveMemberName(
        const source::Location& loc,
        Member* scope,
        const llvm::StringRef& name,
        NameLookupResultRef& result);

  private:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    tempest::support::BumpPtrAllocator* _alloc = nullptr;

    void visitAttributes(LookupScope* scope, Defn* defn, const ast::Defn* ast);
    void visitTypeParams(LookupScope* scope, GenericDefn* defn);
    Expr* createNameRef(
        const source::Location& loc,
        const NameLookupResultRef& result,
        Defn* subject,
        Expr* stem = nullptr,
        bool preferPrivate = false);
    void eagerResolveBaseTypes(TypeDefn* td);
    void resolveBaseTypes(LookupScope* scope, TypeDefn* td);
    Type* simplifyTypeSpecialization(SpecializedDefn* specDefn);

    // Return true if target is visible from within currentScope.
    bool isVisibleMember(Member* subject, Member* target);
    bool isVisible(Defn* subject, Defn* target);

    /** Make a copy of this string within the current alloc. */
    llvm::StringRef copyOf(llvm::StringRef str) {
      auto data = static_cast<char*>(_alloc->Allocate(str.size(), 1));
      std::copy(str.begin(), str.end(), data);
      return llvm::StringRef((char*) data, str.size());
    }
  };
}

#endif
