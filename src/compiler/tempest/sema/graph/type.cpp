#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"

namespace tempest::sema::graph {

  Type Type::ERROR(Type::Kind::INVALID);
  Type Type::NO_RETURN(Type::Kind::NEVER);
  Type Type::NOT_EXPR(Type::Kind::NOT_EXPR);
  Type Type::IGNORED(Type::Kind::IGNORED);

    /** The ordinal index of this type variable relative to other type variables. */
  int32_t TypeVar::index() const {
    return param->index();
  }

  const char* Type::KindName(Kind kind) {
    switch (kind) {
      case Kind::INVALID: return "INVALID";
      case Kind::NEVER: return "NEVER";
      case Kind::IGNORED: return "IGNORED";
      case Kind::NOT_EXPR: return "NOT_EXPR";
      case Kind::VOID: return "VOID";
      case Kind::BOOLEAN: return "BOOLEAN";
      case Kind::INTEGER: return "INTEGER";
      case Kind::FLOAT: return "FLOAT";
      case Kind::CLASS: return "CLASS";
      case Kind::STRUCT: return "STRUCT";
      case Kind::INTERFACE: return "INTERFACE";
      case Kind::TRAIT: return "TRAIT";
      case Kind::EXTENSION: return "EXTENSION";
      case Kind::ENUM: return "ENUM";
      case Kind::ALIAS: return "ALIAS";
      case Kind::TYPE_VAR: return "TYPE_VAR";
      case Kind::UNION: return "UNION";
      case Kind::TUPLE: return "TUPLE";
      case Kind::FUNCTION: return "FUNCTION";
      case Kind::MODIFIED: return "MODIFIED";
      case Kind::SPECIALIZED: return "SPECIALIZED";
      case Kind::SINGLETON: return "SINGLETON";
      case Kind::CONTINGENT: return "CONTINGENT";
      case Kind::INFERRED: return "INFERRED";
    }
  }
}
