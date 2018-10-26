#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
#define TEMPEST_SEMA_GRAPH_TRANSFORM_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SPECSTORE_HPP
  #include "tempest/sema/graph/specstore.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::sema::graph {
  class Expr;
  class Member;
}

namespace tempest::sema::transform {
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeArray;
  using tempest::sema::graph::TypeVar;

  /** Abstract base class for type transformations. */
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

  /** Abstract base class for type transformations that uses a type store. */
  class UniqueTypeTransform {
  public:
    UniqueTypeTransform(
        tempest::sema::graph::TypeStore& types,
        tempest::sema::graph::SpecializationStore& specs)
      : _types(types)
      , _specs(specs)
    {}

    const Type* operator()(const Type* in) { return transform(in); }

    const Type* transform(const Type* in);
    virtual const Type* transformTypeVar(const TypeVar* in) { return in; }

    bool transformArray(llvm::SmallVectorImpl<const Type*>& out, const TypeArray& in);

  private:
    tempest::sema::graph::TypeStore& _types;
    tempest::sema::graph::SpecializationStore& _specs;
  };

  /** Abstract base class for non-mutating expression transformations. */
  class ExprTransform {
  public:
    ExprTransform(tempest::support::BumpPtrAllocator& alloc)
      : _alloc(alloc)
    {}

    virtual Expr* transform(Expr* in);
    virtual const Type* transformType(const Type* type) { return type; }
    virtual Member* transformMember(Member* member) { return member; }

    bool transformArray(const ArrayRef<Expr*>& in, llvm::SmallVectorImpl<Expr*>& out);

  private:
    tempest::support::BumpPtrAllocator& _alloc;
  };
}

#endif
