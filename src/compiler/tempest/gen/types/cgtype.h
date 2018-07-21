#ifndef TEMPEST_GEN_TYPES_CGTYPE_H
#define TEMPEST_GEN_TYPES_CGTYPE_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
  #include "tempest/sema/graph/type.h"
#endif

#ifndef LLVM_IR_MODULE_H
  #include <llvm/IR/Module.h>
#endif

namespace tempest::gen::types {
  using tempest::sema::graph::Type;

  /** Represents and generates IR types. */
  class CGType {
  public:
    // CGType(const Type* type)
    //   : _type(type)
    //   , _irType(irType)
    // {}

    CGType(const Type* type, llvm::Type* irType)
      : _type(type)
      , _irType(irType)
      , _irMemberType(irType)
    {}

    CGType(const Type* type, llvm::Type* irType, llvm::Type* irMemberType)
      : _type(type)
      , _irType(irType)
      , _irMemberType(irMemberType)
    {}

    /** The semantic graph type corresponding to this type. */
    const Type* type() const { return _type; }

    /** The IR type. */
    llvm::Type* irType() const { return _irType; }

    /** The IR type used when this type is used as a member. */
    llvm::Type* irMemberType() const { return _irMemberType; }

  private:
    const Type* _type;
    llvm::Type* _irType;
    llvm::Type* _irMemberType;
  };
}

#endif
