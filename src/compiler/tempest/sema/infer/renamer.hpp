#ifndef TEMPEST_SEMA_INFER_RENAMER_HPP
#define TEMPEST_SEMA_INFER_RENAMER_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
  #include "tempest/sema/graph/transform.hpp"
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

namespace tempest::sema::infer {

  /** Given a series of type expressions, renames all type variables with a unique index.
      Type variables originating from the same function or type overload will have the same
      index. */
  class TypeVarRenamer : public TypeTransform {
  public:
    TypeVarRenamer(llvm::BumpPtrAllocator& alloc) : TypeTransform(alloc) {}

    /** Transform a type expression by renaming all of the type variables in it. */
    const Type* transformTypeVar(const TypeVar* tvar);

  private:
    llvm::StringMap<size_t> _nextIndexForName;
  };
}

#endif
