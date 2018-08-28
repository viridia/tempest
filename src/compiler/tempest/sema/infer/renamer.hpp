#ifndef TEMPEST_SEMA_INFER_RENAMER_HPP
#define TEMPEST_SEMA_INFER_RENAMER_HPP 1

#ifndef TEMPEST_SEMA_INFER_TYPES_HPP
  #include "tempest/sema/infer/types.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TRANSFORM_HPP
  #include "tempest/sema/graph/transform.hpp"
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

namespace tempest::sema::infer {
  class ConstraintSolver;

  /** Hash key to uniquely identify an inferred type parameter. */
  typedef std::pair<const TypeParameter*, const OverloadCandidate*> InferredTypeKey;

  inline void hash_combine(size_t& lhs, size_t rhs) {
    lhs ^= rhs + 0x9e3779b9 + (lhs<<6) + (lhs>>2);
  }

  struct InferredTypeKeyHash {
    inline std::size_t operator()(const InferredTypeKey& value) const {
      std::size_t seed = std::hash<const TypeParameter*>()(value.first);
      hash_combine(seed, std::hash<const OverloadCandidate*>()(value.second));
      return seed;
    }
  };

  /** Given a series of type expressions, renames all type variables with a unique index.
      Type variables originating from the same function or type overload will have the same
      index. */
  class TypeVarRenamer : public TypeTransform {
  public:
    TypeVarRenamer(ConstraintSolver& cs);
    TypeVarRenamer(const TypeVarRenamer&) = delete;

    /** Set the current overload context. */
    void setContext(OverloadCandidate* oc) { _context = oc; }

    /** Transform a type expression by renaming all of the type variables in it. */
    const Type* transformTypeVar(const TypeVar* tvar);

    /** Create a type variable from a type parameter and an overload candidate. */
    const InferredType* createInferredType(TypeParameter* param);

  private:
    ConstraintSolver& _cs;
    llvm::StringMap<size_t> _nextIndexForName;
    OverloadCandidate* _context = nullptr;
    std::unordered_map<InferredTypeKey, InferredType*, InferredTypeKeyHash> _inferredVars;
  };
}

#endif
