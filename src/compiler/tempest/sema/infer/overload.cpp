#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/infer/overload.hpp"

namespace tempest::sema::infer {
  using tempest::error::diag;

  OverloadCandidate::~OverloadCandidate() {
    // Delete any inferred types (and the tables they contain) that got allocated.
    // Everything else can be left since it's pool-allocated.
    for (auto type : typeArgs) {
      if (type && type->kind == Type::Kind::INFERRED) {
        delete type;
      }
    }
  }


  bool CallCandidate::isEqualOrNarrower(CallCandidate* cc) {
    size_t numArgs = cast<CallSite>(site)->argList.size();
    for (size_t i = 0; i < numArgs; i++) {
      auto thisParam = paramTypes[paramAssignments[i]];
      auto otherParam = cc->paramTypes[cc->paramAssignments[i]];
      if (!convert::isEqualOrNarrower(thisParam, otherParam)) {
        return false;
      }
    }
    if (!convert::isEqualOrNarrower(returnType, cc->returnType)) {
      return false;
    }

    // TODO: Check for unassigned params (either varargs or default params).
    return true;
  }
}
