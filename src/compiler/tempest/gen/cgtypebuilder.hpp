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

namespace llvm {
  class Constant;
  class DataLayout;
  class IntegerType;
  class StructType;
}

namespace tempest::gen {
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UnionType;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::SpecializationKey;
  using tempest::sema::graph::SpecializationKeyHash;

  /** Description of a union type. */
  class CGUnionType {
  public:
    // Common tag values. Tags for value types start at 3. */
    enum Tag {
      VOID = 0,
      REF = 1,
      IFACE = 2,
      VALUE_START = 3,
    };

    llvm::Type* type;
    llvm::Type* ssaType;
    llvm::IntegerType* tagType = nullptr;
    llvm::Type* valueType = nullptr;
    bool hasRefType = false;
    bool hasInterfaceType = false;
    bool hasVoidType = false;

    SmallVector<const Type*, 4> valueTypes;
    size_t valueStructIndex = 0;
  };

  /** Maps Tempest type expressions to LLVM types. */
  class CGTypeBuilder {
  public:
    CGTypeBuilder(llvm::LLVMContext& context, const llvm::DataLayout* dataLayout)
      : _context(context)
      , _dataLayout(dataLayout)
    {}

    llvm::Type* get(const Type*, ArrayRef<const Type*> typeArgs);
    llvm::Type* getMemberType(const Type*, ArrayRef<const Type*> typeArgs);
    CGUnionType* createUnion(const UnionType* ut);

    /** References to built-in types. */
    llvm::Type* getObjectType();
    llvm::StructType* getClassDescType();
    llvm::StructType* getInterfaceDescType();
    llvm::StructType* getClassInterfaceTransType();

  private:
    llvm::Type* createClass(const UserDefinedType*, ArrayRef<const Type*> typeArgs);

    typedef std::unordered_map<
        SpecializationKey<Type>, llvm::Type*, SpecializationKeyHash<Type>> TypeMap;
    typedef std::unordered_map<
        SpecializationKey<TypeDefn>, llvm::Type*, SpecializationKeyHash<TypeDefn>> TypeDefnMap;
    typedef std::unordered_map<const UnionType*, std::unique_ptr<CGUnionType>> UnionMap;

    llvm::LLVMContext& _context;
    const llvm::DataLayout* _dataLayout;

    TypeMap _types;
    TypeDefnMap _typeDefns;
    UnionMap _unions;

    llvm::Type* _objectType = nullptr;
    llvm::StructType* _classDescType = nullptr;
    llvm::StructType* _interfaceDescType = nullptr;
    llvm::StructType* _classInterfaceTransType = nullptr;
  };
}

#endif
