#ifndef TEMPEST_GEN_CGDEBUGTYPEBUILDER_H
#define TEMPEST_GEN_CGDEBUGTYPEBUILDER_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
  #include "tempest/sema/graph/type.h"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_H
  #include "tempest/source/location.h"
#endif

#ifndef LLVM_IR_DIBUILDER_H
  #include <llvm/IR/DIBuilder.h>
#endif

#include <unordered_map>

namespace tempest::gen {
  using tempest::source::ProgramSource;
  using tempest::sema::graph::FunctionType;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::ValueDefn;

  /** Maps Tempest type expressions to LLVM DebugInfo types. */
  class CGDebugTypeBuilder {
  public:
    CGDebugTypeBuilder(llvm::DIBuilder& builder, llvm::DICompileUnit* diCompileUnit)
      : _builder(builder)
      , _diCompileUnit(diCompileUnit)
    {}

    /** Set the compilation unit after construction. */
    void setCompileUnit(llvm::DICompileUnit* diCompileUnit) { _diCompileUnit = diCompileUnit; }

    llvm::DIType* get(const Type*);
    llvm::DIType* getMemberType(const Type*);

    llvm::DISubroutineType* createFunctionType(const FunctionType* ft);
    llvm::DICompositeType* createClass(const UserDefinedType*);
    llvm::DIDerivedType* createMember(const ValueDefn*);

    /** Get a file from a program source. */
    llvm::DIFile* getDIFile(ProgramSource* src);
  private:

    typedef std::unordered_map<const Type*, llvm::DIType*> TypeMap;
    typedef std::unordered_map<const TypeDefn*, llvm::DIType*> TypeDefnMap;

    llvm::DIBuilder& _builder;
    TypeMap _types;
    TypeDefnMap _typeDefns;
    llvm::DICompileUnit* _diCompileUnit;
  };
}

#endif
