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
        Env& env)
      : UniqueTypeTransform(types, specs)
      , _env(env)
    {}

    const Type* transformTypeVar(const TypeVar* in) {
      assert(_env.has(in));
      return _env.get(in);
    }

  private:
    Env& _env;
  };
}

#endif
