#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
#define TEMPEST_SEMA_GRAPH_TRANSFORM_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::sema::graph {
  using tempest::sema::graph::Type;

  /** Helper class that takes a type expression and returns a corresponding type expression
      where all type variables have been replaced. */
  class TypeTransform {
  public:
    TypeTransform(tempest::support::BumpPtrAllocator& alloc) : _alloc(alloc) {}

    const Type* transform(const Type* in);
    virtual const Type* transformTypeVar(const TypeVar* in) { return in; }
    virtual const Type* transformContingentType(const infer::ContingentType* in);
    virtual const Type* transformInferredType(const infer::InferredType* in) { return in; }

    bool transformArray(llvm::SmallVectorImpl<const Type*>& out, const TypeArray& in);

  private:
    tempest::support::BumpPtrAllocator& _alloc;
  };
}

#endif
