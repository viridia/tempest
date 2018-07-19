// ============================================================================
// gen/cgglobal.h: base class for global symbols.
// ============================================================================

#ifndef TEMPEST_GEN_CGGLOBAL_H
#define TEMPEST_GEN_CGGLOBAL_H 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_H
  #include "tempest/sema/graph/type.h"
#endif

#ifndef LLVM_IR_MODULE_H
  #include <llvm/IR/Module.h>
#endif

namespace tempest::gen {
  /** Code gen node for a global symbol. */
  class CGGlobal {
  public:
    CGGlobal(Type* type);

    /** Data type of this global symbol. */
    Type* type() const { return _type; }

    /** Name of this global symbol. */
    const llvm::StringRef& name() const { return _name; }
    void setName(const llvm::StringRef& name) { _name = name; }

    /** True if this symbol is external to its parent module. */
    bool isExternal() const { return _external; }
    void setExternal(bool external) { _external = external; }

  private:
    Type* _type;
    llvm::StringRef _name;
    bool _external;
  };
}

#endif
