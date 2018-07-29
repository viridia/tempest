#ifndef TEMPEST_GEN_CGDEBUGTYPEBUILDER_HPP
#define TEMPEST_GEN_CGDEBUGTYPEBUILDER_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#ifndef LLVM_IR_DIBUILDER_H
  #include <llvm/IR/DIBuilder.h>
#endif

#ifndef LLVM_IR_DATALAYOUT_H
  #include <llvm/IR/DataLayout.h>
#endif

#include <unordered_map>

namespace tempest::gen {
  using tempest::source::ProgramSource;
  using tempest::sema::graph::FunctionType;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::ValueDefn;

  class CGTypeBuilder;

  /** Maps Tempest type expressions to LLVM DebugInfo types. */
  class CGDebugTypeBuilder {
  public:
    CGDebugTypeBuilder(
        llvm::DIBuilder& builder,
        llvm::DICompileUnit* diCompileUnit,
        CGTypeBuilder& typeBuilder)
      : _builder(builder)
      , _typeBuilder(typeBuilder)
      , _diCompileUnit(diCompileUnit)
    {}

    /** Set the compilation unit after construction. */
    void setCompileUnit(llvm::DICompileUnit* diCompileUnit) { _diCompileUnit = diCompileUnit; }
    void setDataLayout(const llvm::DataLayout* dataLayout) { _dataLayout = dataLayout; }

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
    CGTypeBuilder& _typeBuilder;
    TypeMap _types;
    TypeDefnMap _typeDefns;
    llvm::DICompileUnit* _diCompileUnit;
    const llvm::DataLayout* _dataLayout = nullptr;
  };
}

#endif
