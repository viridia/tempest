#ifndef TEMPEST_GEN_CGTYPEBUILDER_HPP
#define TEMPEST_GEN_CGTYPEBUILDER_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SPECKEY_HPP
  #include "tempest/sema/graph/speckey.hpp"
#endif

#ifndef LLVM_IR_TYPE_H
  #include <llvm/IR/Type.h>
#endif

#include <unordered_map>

namespace tempest::gen {
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::SpecializationKey;
  using tempest::sema::graph::SpecializationKeyHash;

  /** Maps Tempest type expressions to LLVM types. */
  class CGTypeBuilder {
  public:
    CGTypeBuilder(llvm::LLVMContext& context) : _context(context) {}

    llvm::Type* get(const Type*, ArrayRef<const Type*> typeArgs);
    llvm::Type* getMemberType(const Type*, ArrayRef<const Type*> typeArgs);

  private:
    llvm::Type* createClass(const UserDefinedType*, ArrayRef<const Type*> typeArgs);

    typedef std::unordered_map<
        SpecializationKey<Type>, llvm::Type*, SpecializationKeyHash<Type>> TypeMap;
    typedef std::unordered_map<
        SpecializationKey<TypeDefn>, llvm::Type*, SpecializationKeyHash<TypeDefn>> TypeDefnMap;

    llvm::LLVMContext& _context;
    TypeMap _types;
    TypeDefnMap _typeDefns;
  };
}

#endif
