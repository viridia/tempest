#ifndef TEMPEST_SEMA_GRAPH_ENV_HPP
#define TEMPEST_SEMA_GRAPH_ENV_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

namespace tempest::sema::graph {
  class Member;
  class Type;
  class TypeVar;
  class TypeParameter;
  class SpecializedDefn;

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

    /** Initialize from a specialization. */
    void fromSpecialization(SpecializedDefn* sd);

    /** If a member is specialized, then return the generic definition and set the environment
        to the type arguments of the specialization. */
    Member* unwrap(Member* m);
  };
}

#endif
