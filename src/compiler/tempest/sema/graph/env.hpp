#ifndef TEMPEST_SEMA_GRAPH_ENV_HPP
#define TEMPEST_SEMA_GRAPH_ENV_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

namespace tempest::sema::graph {
  class Type;
  class TypeVar;
  class TypeParameter;

  /** Class used to temporarily contain a mapping from type parameter to type argument. */
  struct Env {
    llvm::ArrayRef<TypeParameter*> params;
    llvm::SmallVector<const Type*, 4> args;

    /** True if this type variable is included in the environment. */
    bool has(const TypeVar* tvar);
    bool has(const TypeParameter* param);

    /** Return the mapped value of this type parameter. */
    const Type* get(const TypeVar* tvar);
    const Type* get(const TypeParameter* param);
  };
}

#endif
