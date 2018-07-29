// ============================================================================
// formatters.h: Various formatting helpers.
// ============================================================================

#ifndef TEMPEST_ERROR_FORMATTERS_HPP
#define TEMPEST_ERROR_FORMATTERS_HPP 1

// #ifndef TEMPEST_SEMGRAPH_TYPEVISITOR_HPP
//   #include "spark/semgraph/typevisitor.h"
// #endif

#include <sstream>

namespace tempest::error {
// using semgraph::Type;

// /** Format helper for type expressions. */
// class FormattedType : public semgraph::TypeVisitor<void, ::std::ostream&> {
// public:
//   FormattedType(Type* type) : _type(type) {}

//   void format(::std::ostream& os) const { const_cast<FormattedType*>(this)->exec(_type, os); }

//   void visitType(Type *t, ::std::ostream& os);
// private:
//   Type* _type;
// };

// inline FormattedType formatted(Type* t) { return FormattedType(t); }

// // Format helper for types.
// inline ::std::ostream& operator<<(::std::ostream& os, const FormattedType& ft) {
//   ft.format(os);
//   return os;
// }

}

#endif
