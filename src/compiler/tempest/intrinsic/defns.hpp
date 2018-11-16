#ifndef TEMPEST_INTRINSIC_DEFNS_HPP
#define TEMPEST_INTRINSIC_DEFNS_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::intrinsic {
  using tempest::sema::graph::Defn;
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::SymbolTable;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::TypeStore;
  using tempest::sema::graph::ValueDefn;

  struct PrimitiveOperators {
    std::unique_ptr<FunctionDefn> add;
    std::unique_ptr<FunctionDefn> subtract;
    std::unique_ptr<FunctionDefn> multiply;
    std::unique_ptr<FunctionDefn> divide;
    std::unique_ptr<FunctionDefn> remainder;
    std::unique_ptr<FunctionDefn> lsh;
    std::unique_ptr<FunctionDefn> rsh;
    std::unique_ptr<FunctionDefn> bitOr;
    std::unique_ptr<FunctionDefn> bitAnd;
    std::unique_ptr<FunctionDefn> bitXor;
    std::unique_ptr<FunctionDefn> lt;
    std::unique_ptr<FunctionDefn> le;
    std::unique_ptr<FunctionDefn> uminus;
    std::unique_ptr<FunctionDefn> comp;
  };

  /** Class to contain all of the various intrinsic definitions. */
  class IntrinsicDefns {
  public:
    IntrinsicDefns();

    std::unique_ptr<SymbolTable> builtinScope;
    std::unique_ptr<TypeDefn> objectClass;
    std::unique_ptr<TypeDefn> throwableClass;
    std::unique_ptr<TypeDefn> flexAllocClass;
    TypeDefn* iterableType = nullptr;
    TypeDefn* iteratorType = nullptr;
    TypeDefn* addressType = nullptr;

    PrimitiveOperators intOps;
    PrimitiveOperators uintOps;
    PrimitiveOperators floatOps;

    // Equality intrinsic
    std::unique_ptr<FunctionDefn> eq;

    // Register an externally-declared intrinsic
    bool registerExternal(Defn* d);

    // Singleton getter.
    static IntrinsicDefns* get();

  private:
    TypeStore _types;
    std::unique_ptr<TypeDefn> makeTypeDefn(Type::Kind kind, llvm::StringRef name);
    std::unique_ptr<FunctionDefn> makeInfixOp(
        llvm::StringRef name, Type* argType, IntrinsicFn intrinsic);
    std::unique_ptr<FunctionDefn> makeUnaryOp(
        llvm::StringRef name, Type* argType, IntrinsicFn intrinsic);
    std::unique_ptr<FunctionDefn> makeRelationalOp(
        llvm::StringRef name, Type* argType, IntrinsicFn intrinsic);
    ValueDefn* addValueDefn(
        std::unique_ptr<TypeDefn>& td,
        Member::Kind kind,
        llvm::StringRef name);

    static IntrinsicDefns* instance;
  };
}

#endif
