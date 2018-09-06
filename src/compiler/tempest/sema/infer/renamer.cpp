#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/renamer.hpp"

namespace tempest::sema::infer {

  TypeVarRenamer::TypeVarRenamer(ConstraintSolver& cs) : TypeTransform(cs.alloc()), _cs(cs) {}

  const InferredType* TypeVarRenamer::createInferredType(TypeParameter* param) {
    InferredTypeKey key(param, _context);
    auto it = _inferredVars.find(key);
    if (it != _inferredVars.end()) {
      return it->second;
    }

    // Calculate a new unique index for this name in this context
    size_t index = 1;
    auto nt = _nextIndexForName.find(param->name());
    if (nt != _nextIndexForName.end()) {
      index = nt->second++;
    } else {
      _nextIndexForName[param->name()] = index;
    }

    auto ivar = new (_cs.alloc()) InferredType(param, &_cs);
    ivar->index = index;
    _inferredVars[key] = ivar;
    // self.contextForParamIndex[(param, index)] = context
    // self.typeVarIds.add(id(tvar))
    return ivar;
  }

  const Type* TypeVarRenamer::transformTypeVar(const TypeVar* tvar) {
    // TODO: Make sure we only rename type vars that are part of the curent context!
    assert(false && "Implement");
  }
}
