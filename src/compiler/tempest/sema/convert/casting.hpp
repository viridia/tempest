#ifndef TEMPEST_SEMA_CONVERT_CONVERT_HPP
#define TEMPEST_SEMA_CONVERT_CONVERT_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;

  /** Adds appropriate type casts to an expression if needed. Does minimal error checking,
      that is assumed to have been handled earlier in the type resolution pass.
   */
  class TypeCasts {
  public:
    TypeCasts(tempest::support::BumpPtrAllocator& alloc) : _alloc(alloc) {}

    Expr* cast(Type* dst, Expr* src);

  private:
    tempest::support::BumpPtrAllocator& _alloc;

    Expr* makeCast(Expr::Kind kind, Expr* arg, Type* type);
  };
}

#endif
