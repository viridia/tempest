#ifndef TEMPEST_SEMA_TRANSFORM_APPLYSPEC_HPP
#define TEMPEST_SEMA_TRANSFORM_APPLYSPEC_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_TRANSFORM_TRANSFORM_HPP
  #include "tempest/sema/transform/transform.hpp"
#endif

namespace tempest::sema::transform {
  using namespace tempest::sema::graph;

  /** Helper class that accepts a type and returns that type with all type variables replaced
      by the types they are bound to. So for example, if the type arguments are T -> i32, and
      the input type is T | void, then the output is i32 | void.

      This class retains ownership of any types thus created; They will go away when this class
      does.
  */
  class TempMapTypeVars : public TypeTransform {
  public:
    TempMapTypeVars(llvm::ArrayRef<const Type*> typeArgs)
      : TypeTransform(_alloc)
      , _typeArgs(typeArgs)
    {}

    const Type* transformTypeVar(const TypeVar* in) {
      return _typeArgs[static_cast<const TypeVar*>(in)->index()];
    }

  private:
    tempest::support::BumpPtrAllocator _alloc;
    llvm::ArrayRef<const Type*> _typeArgs;
  };
}

#endif
