#ifndef TEMPEST_SEMA_TRANSFORM_MAPENV_HPP
#define TEMPEST_SEMA_TRANSFORM_MAPENV_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_TRANSFORM_TRANSFORM_HPP
  #include "tempest/sema/transform/transform.hpp"
#endif

namespace tempest::sema::transform {
  using namespace tempest::sema::graph;

  /** Map a type expression through an environment. This class is relatively smart in that
      type expressions that don't have any type variables will be returned unchanged.

      Newly-created type expressions will be registed with / owned by the TypeStore or
      SpecializationStore as needed.
  */
  class MapEnvTransform : public UniqueTypeTransform {
  public:
    MapEnvTransform(
        tempest::sema::graph::TypeStore& types,
        tempest::sema::graph::SpecializationStore& specs,
        llvm::ArrayRef<TypeParameter*> params,
        ArrayRef<const Type*> typeArgs)
      : UniqueTypeTransform(types, specs)
      , _params(params)
      , _typeArgs(typeArgs)
    {
      assert(params.size() == typeArgs.size());
    }

    const Type* transformTypeVar(const TypeVar* in) {
      assert(std::find(_params.begin(), _params.end(), in->param) != _params.end());
      auto index = in->param->index();
      return _typeArgs[index];
    }

  private:
    llvm::ArrayRef<TypeParameter*> _params;
    ArrayRef<const Type*> _typeArgs;
  };
}

#endif
