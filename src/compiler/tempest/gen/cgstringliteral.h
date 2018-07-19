// ============================================================================
// gen/cgstringliteral.h: generated string literals.
// ============================================================================

#ifndef TEMPEST_GEN_CGSTRINGLITERAL_H
#define TEMPEST_GEN_CGSTRINGLITERAL_H 1

#ifndef TEMPEST_GEN_CGGLOBAL_H
  #include "tempest/gen/cgglobal.h"
#endif

namespace tempest::gen {
  /** Code gen node for a string literal. */
  class CGStringLiteral : public CGGlobal {
  public:
    CGStringLiteral(Type* type);

    /** Value of the string. */
    const llvm::StringRef& value() const { return _value; }
    void setName(const llvm::StringRef& value) { _value = value; }

  private:
    llvm::StringRef _value;
  };
}

#endif
