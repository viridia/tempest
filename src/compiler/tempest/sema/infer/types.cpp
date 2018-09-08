#include "tempest/sema/infer/constraintsolver.hpp"
#include "tempest/sema/infer/types.hpp"

namespace tempest::sema::infer {
  bool InferredType::isViable(const Constraint& c) const {
    return solver->isViable(c.when);
  }

  ::std::ostream& operator<<(::std::ostream& os, const ShowConstraints& sc) {
    if (auto inferred = dyn_cast<InferredType>(sc.type)) {
      for (size_t i = 0; i < inferred->constraints.size(); i += 1) {
        if (i > 0) {
          if (i == inferred->constraints.size() - 1) {
            os << " or ";
          } else {
            os << ", ";
          }
        }
        format(os, inferred->constraints[i].value);
      }
    } else {
      format(os, sc.type);
    }
    return os;
  }
}
