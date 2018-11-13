#ifndef TEMPEST_SEMA_PASS_FINDOVERRIDES_HPP
#define TEMPEST_SEMA_PASS_FINDOVERRIDES_HPP 1

#ifndef TEMPEST_COMPILER_COMPILATIONUNIT_HPP
  #include "tempest/compiler/compilationunit.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

namespace tempest::sema::pass {
  using namespace tempest::sema::graph;
  using tempest::compiler::CompilationUnit;

  enum InheritanceType : uint8_t {
    DEFINED,
    INHERIT_EXTENDS,
    INHERIT_IMPLEMENTS,
  };

  template<class T>
  struct Inherited {
    // Whether the method comes from extends or implemnts. */
    InheritanceType itype;

    // Which base class (0..N)
    size_t baseIndex;

    // Pointer to member defn
    T* memberDefn;

    // Type arguments
    ArrayRef<const Type*> typeArgs;
  };

  typedef llvm::StringMap<std::vector<Inherited<Defn>>> InheritedMethodMap;

  /** Pass which resolves references to names. */
  class FindOverridesPass {
  public:
    FindOverridesPass(CompilationUnit& cu) : _cu(cu) {}

    void run();

    /** Process a single module. */
    void process(Module* mod);

    // Defns

    void visitList(DefnArray members);
    void visitTypeDefn(TypeDefn* td);
    void visitCompositeDefn(TypeDefn* td);

  private:
    CompilationUnit& _cu;
    size_t _sourcesProcessed = 0;
    size_t _importSourcesProcessed = 0;
    tempest::support::BumpPtrAllocator* _alloc = nullptr;

    void visitMembers(const DefnList& members, TypeDefn* td);
    void findSameNamedOverrides(
        TypeDefn* td,
        SmallVectorImpl<FunctionDefn*>& methods,
        SmallVectorImpl<Inherited<FunctionDefn>>& inherited);
    void fillEmptySlots(
        TypeDefn* td,
        SmallVectorImpl<Inherited<FunctionDefn>>& inherited);
    void appendNewMethods(TypeDefn* td);
    bool findEqualSignatures(SmallVectorImpl<FunctionDefn*>& methods);
  };
}

#endif
