#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
#define TEMPEST_SEMA_GRAPH_TRANSFORM_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

namespace tempest::sema::graph {
  using tempest::sema::graph::Type;

  /** Helper class that takes a type expression and returns a corresponding type expression
      where all type variables have been replaced. */
  class TypeTransform {
  public:
    TypeTransform(llvm::BumpPtrAllocator& alloc) : _alloc(alloc) {}

    const Type* transform(const Type* in);
    virtual const Type* transformTypeVar(const TypeVar* in) { return in; }
    bool transformArray(llvm::SmallVectorImpl<const Type*>& out, const TypeArray& in);

  private:
    template<class T> llvm::ArrayRef<T> copyOf(const llvm::SmallVectorImpl<T>& array) {
      auto data = static_cast<T*>(_alloc.Allocate(array.size(), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    llvm::BumpPtrAllocator& _alloc;
  };
}

#endif
