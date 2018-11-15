#ifndef TEMPEST_INTRINSIC_BUILDER_HPP
#define TEMPEST_INTRINSIC_BUILDER_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_PRIMITIVETYPE_HPP
  #include "tempest/sema/graph/primitivetype.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPESTORE_HPP
  #include "tempest/sema/graph/typestore.hpp"
#endif

#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
  #include "tempest/support/allocator.hpp"
#endif

namespace tempest::intrinsic {
  using namespace tempest::sema::graph;
  using tempest::source::Location;

  class BuiltinMethodBuilder {
  public:
    BuiltinMethodBuilder(TypeStore& types, std::unique_ptr<TypeDefn>& parent, StringRef name)
      : _types(types)
    {
      _parent = parent.get();
      _method = new FunctionDefn(Location(), name, _parent);
      _method->allTypeParams().assign(
          parent->allTypeParams().begin(), parent->allTypeParams().end());
    }

    BuiltinMethodBuilder& setStatic() {
      _method->setStatic(true);
      return *this;
    }

    BuiltinMethodBuilder& constructor() {
      _method->setConstructor(true);
      return *this;
    }

    BuiltinMethodBuilder& intrinsic(IntrinsicFn ifn) {
      _method->setIntrinsic(ifn);
      return *this;
    }

    BuiltinMethodBuilder& addTypeParam(TypeParameter* param) {
      param->setIndex(_method->allTypeParams().size());
      _method->typeParams().push_back(param);
      _method->allTypeParams().push_back(param);
      return *this;
    }

    BuiltinMethodBuilder& addParam(StringRef name, const Type* type) {
      auto param = new (_types.alloc()) ParameterDefn(
          Location(), name, _method, type);
      _method->params().push_back(param);
      _paramTypes.push_back(type);
      return *this;
    }

    BuiltinMethodBuilder& returnType(const Type* rt) {
      _returnType = rt;
      return *this;
    }

    FunctionDefn* build() {
      if (!_method->isStatic()) {
        _method->setSelfType(_parent->type());
      }
      _method->setType(_types.createFunctionType(
          _returnType, _paramTypes, false));
      _parent->members().push_back(_method);
      _parent->memberScope()->addMember(_method);
      return _method;
    }

  private:
    TypeStore& _types;
    TypeDefn* _parent;
    FunctionDefn* _method;
    SmallVector<const Type*, 4> _paramTypes;
    const Type* _returnType = &VoidType::VOID;
  };
}

#endif
