#ifndef TEMPEST_GEN_CGTYPEBUILDER_H
#define TEMPEST_GEN_CGTYPEBUILDER_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
  #include "tempest/sema/graph/type.h"
#endif

#ifndef LLVM_IR_TYPE_H
  #include <llvm/IR/Type.h>
#endif

#include <unordered_map>

namespace tempest::gen {
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;

  /** Maps Tempest type expressions to LLVM types. */
  class CGTypeBuilder {
  public:
    CGTypeBuilder(llvm::LLVMContext& context) : _context(context) {}

    llvm::Type* get(const Type*);
    llvm::Type* getMemberType(const Type*);

  private:
    llvm::Type* createClass(const UserDefinedType*);

    typedef std::unordered_map<const Type*, llvm::Type*> TypeMap;
    typedef std::unordered_map<const TypeDefn*, llvm::Type*> TypeDefnMap;

    llvm::LLVMContext& _context;
    TypeMap _types;
    TypeDefnMap _typeDefns;
  };
}

#endif
