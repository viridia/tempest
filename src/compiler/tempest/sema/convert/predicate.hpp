#ifndef TEMPEST_SEMA_CONVERT_PREDICATE_HPP
#define TEMPEST_SEMA_CONVERT_PREDICATE_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
  #include "tempest/sema/convert/result.hpp"
#endif

namespace tempest::sema::convert {
  using namespace tempest::sema::graph;

  /** Class used to temporarily contain a mapping from type parameter to type argument. */
  struct ConversionEnv {
    llvm::ArrayRef<TypeParameter*> params;
    llvm::SmallVector<const Type*, 4> args;

    /** True if this type variable is included in the environment. */
    bool has(const TypeVar* tvar) {
      return std::find(params.begin(), params.end(), tvar->param) != params.end();
    }
  };

  /** Return whether the source type is assignable to the destination type. */
  ConversionResult isAssignable(const Type* dst, const Type* src);

  /** Return whether the source type is assignable to the destination type. */
  ConversionResult isAssignable(
      const Type* dst, uint32_t dstMods, ConversionEnv& dstEnv,
      const Type* src, uint32_t srcMods, ConversionEnv& srcEnv);

  /** Return whether the source type is assignable to the destination type. */
  inline ConversionResult isAssignable(
      const Type* dst, ConversionEnv& dstEnv,
      const Type* src, ConversionEnv& srcEnv) {
    return isAssignable(dst, 0, dstEnv, src, 0, srcEnv);
  }

  /** Return whether the source type is equal to the destination type. */
  bool isEqual(const Type* dst, const Type* src);

  /** Return whether the source type is equal to the destination type. */
  bool isEqual(
      const Type* dst, uint32_t dstMods, ConversionEnv& dstEnv,
      const Type* src, uint32_t srcMods, ConversionEnv& srcEnv);

  /** Return whether the subtype (st) type is a subtype of the base type (bt). */
  bool isSubtype(
      const TypeDefn* st, ConversionEnv& stEnv,
      const TypeDefn* bt, ConversionEnv& btEnv);

  /** Similar to isSubtype, but takes into account upper and lower bounds on type variables.
      Returns true if both sides are equally specific. */
  bool isEqualOrNarrower(const Type* lt, const Type* rt);
  bool isEqualOrNarrower(const Type* lt, ConversionEnv& lEnv, const Type* rt, ConversionEnv& rEnv);

  /** Return whether the left type implements all the members of the right type. */
  bool implementsMembers(
      const TypeDefn* lt, ConversionEnv& lEnv,
      const TypeDefn* rt, ConversionEnv& rEnv);

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
