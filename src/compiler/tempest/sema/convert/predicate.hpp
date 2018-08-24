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

  /** Return whether the source type is a subtype of the destination type. */
  bool isSubtype(
      const TypeDefn* dst, ConversionEnv& dstEnv,
      const TypeDefn* src, ConversionEnv& srcEnv);

  /** Return whether the source type implements all the members of the destination type. */
  bool implementsMembers(
      const TypeDefn* dst, ConversionEnv& dstEnv,
      const TypeDefn* src, ConversionEnv& srcEnv);

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
