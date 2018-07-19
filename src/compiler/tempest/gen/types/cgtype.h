// ============================================================================
// gen/cgstringliteral.h: generated string literals.
// ============================================================================

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
    CGType(Type* type);

    /** The semantic graph type corresponding to this type. */
    Type* type() const { return _type; }

    /** The IR type. */
    llvm::Type* irType() const { return _irType; }
    // Add irFieldType for when it is used as a field.

  private:
    Type* _type;
    llvm::Type* _irType;
  };
}

#endif
