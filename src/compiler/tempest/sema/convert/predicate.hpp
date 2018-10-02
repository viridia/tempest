#ifndef TEMPEST_SEMA_CONVERT_PREDICATE_HPP
#define TEMPEST_SEMA_CONVERT_PREDICATE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_ENV_HPP
  #include "tempest/sema/graph/env.hpp"
#endif

#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
  #include "tempest/sema/convert/result.hpp"
#endif

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;

  /** Return whether the source type is assignable to the destination type. */
  ConversionResult isAssignable(const Type* dst, const Type* src);

  /** Return whether the source type is assignable to the destination type. */
  ConversionResult isAssignable(
      const Type* dst, uint32_t dstMods, Env& dstEnv,
      const Type* src, uint32_t srcMods, Env& srcEnv);

  /** Return whether the source type is assignable to the destination type. */
  inline ConversionResult isAssignable(
      const Type* dst, Env& dstEnv,
      const Type* src, Env& srcEnv) {
    return isAssignable(dst, 0, dstEnv, src, 0, srcEnv);
  }

  /** Return whether the source type is equal to the destination type. */
  bool isEqual(const Type* dst, const Type* src);

  /** Return whether the source type is equal to the destination type. */
  bool isEqual(
      const Type* dst, uint32_t dstMods, Env& dstEnv,
      const Type* src, uint32_t srcMods, Env& srcEnv);

  /** Return whether the subtype (st) type is a subtype of the base type (bt). */
  bool isSubtype(
      const TypeDefn* st, Env& stEnv,
      const TypeDefn* bt, Env& btEnv);

  /** Returns true if the left type is equal to or narrower than the right type.
      Similar to isSubtype, but takes into account upper and lower bounds on type variables.
      Returns true if both sides are equally specific. */
  bool isEqualOrNarrower(const Type* lt, const Type* rt);
  bool isEqualOrNarrower(const Type* lt, Env& lEnv, const Type* rt, Env& rEnv);

  /** Return whether the left type implements all the members of the right type. */
  bool implementsMembers(
      const TypeDefn* lt, Env& lEnv,
      const TypeDefn* rt, Env& rEnv);

  /** Return true if the type modifier flags allow a source type to be coerced to a destination
      type. */
  inline bool isCompatibleMods(uint32_t dstMods, uint32_t srcMods) {
    // If one is immutable, then both must be.
    if ((dstMods & ModifiedType::IMMUTABLE) != (srcMods & ModifiedType::IMMUTABLE)) {
      return false;
    }
    // is source is readonly, then dest must also be.
    if ((srcMods & ModifiedType::READ_ONLY) && !(dstMods && ModifiedType::READ_ONLY)) {
      return false;
    }
    return true;
  }
}

#endif
