#include "tempest/sema/graph/expr.hpp"

namespace tempest::sema::graph {
  Expr Expr::ERROR(Expr::Kind::INVALID, Location());

  const char* Expr::KindName(Kind kind) {
    switch (kind) {
      case Kind::INVALID: return "INVALID";
      case Kind::VOID: return "VOID";
      case Kind::ID: return "ID";
      case Kind::SELF: return "SELF";
      case Kind::SUPER: return "SUPER";
      // case Kind::BUILTIN_ATTR: return "BUILTIN_ATTR";

      case Kind::BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
      case Kind::INTEGER_LITERAL: return "INTEGER_LITERAL";
      case Kind::FLOAT_LITERAL: return "FLOAT_LITERAL";
      case Kind::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
      case Kind::STRING_LITERAL: return "STRING_LITERAL";
      case Kind::ARRAY_LITERAL: return "ARRAY_LITERAL";
      case Kind::NOT: return "NOT";
      case Kind::NEGATE: return "NEGATE";
      case Kind::COMPLEMENT: return "COMPLEMENT";
      case Kind::ADD: return "ADD";
      case Kind::SUBTRACT: return "SUBTRACT";
      case Kind::MULTIPLY: return "MULTIPLY";
      case Kind::DIVIDE: return "DIVIDE";
      case Kind::REMAINDER: return "REMAINDER";
      case Kind::LSHIFT: return "LSHIFT";
      case Kind::RSHIFT: return "RSHIFT";
      case Kind::BIT_AND: return "BIT_AND";
      case Kind::BIT_OR: return "BIT_OR";
      case Kind::BIT_XOR: return "BIT_XOR";
      case Kind::EQ: return "EQ";
      case Kind::NE: return "NE";
      case Kind::LT: return "LT";
      case Kind::LE: return "LE";
      case Kind::GT: return "GT";
      case Kind::GE: return "GE";
      case Kind::REF_NE: return "REF_NE";
      case Kind::REF_EQ: return "REF_EQ";
      case Kind::ASSIGN: return "ASSIGN";
    //   case Kind::UP_CAST: return "UP_CAST";
    //   case Kind::DOWN_CAST: return "DOWN_CAST";
    //   case Kind::TRY_CAST: return "TRY_CAST";
    //   case Kind::IFACE_CAST: return "IFACE_CAST";
    //   case Kind::DYN_IFACE_CAST: return "DYN_IFACE_CAST";
    //   case Kind::UNION_CTOR_CAST: return "UNION_CTOR_CAST";
    //   case Kind::UNION_MEMBER_CAST: return "UNION_MEMBER_CAST";
    //   case Kind::BOX_CAST: return "BOX_CAST";
    //   case Kind::UNBOX_CAST: return "UNBOX_CAST";
    // #  case Kind::DYNAMIC_CAST: return "DYNAMIC_CAST";                 # Cast from base/iface to case Kind::subclass: return "subclass"; null if fail
    // #  EXPR_TYPE(QualCast)       // Cast that changes only qualifiers (no effect)
    // #  EXPR_TYPE(UnionMemberCast)// Cast from a union type.
    // #  EXPR_TYPE(CheckedUnionMemberCast)// Cast from a union case Kind::type: return "type"; with type check.
    //   case Kind::REP_CAST: return "REP_CAST";
    //   case Kind::TRUNCATE: return "TRUNCATE";
    //   case Kind::SIGN_EXTEND: return "SIGN_EXTEND";
    //   case Kind::ZERO_EXTEND: return "ZERO_EXTEND";
    //   case Kind::FP_EXTEND: return "FP_EXTEND";
    //   case Kind::FP_TRUNC: return "FP_TRUNC";
    //   case Kind::INT_TO_FLOAT: return "INT_TO_FLOAT";
    //   case Kind::INTERFACE_DATA: return "INTERFACE_DATA";
    // # EXPR_TYPE(BitCast)        // Reinterpret cast

      case Kind::LOGICAL_AND: return "LOGICAL_AND";
      case Kind::LOGICAL_OR: return "LOGICAL_OR";
    //   case Kind::RANGE: return "RANGE";
    //   case Kind::PACK: return "PACK";
    //   case Kind::AS_TYPE: return "AS_TYPE";
    //   case Kind::IS_TYPE: return "IS_TYPE";
    // #  case Kind::IN: return "IN";
    // #  case Kind::NOT_IN: return "NOT_IN";
    //   case Kind::RETURNS: return "RETURNS";
    //   case Kind::PROG: return "PROG";
    //   case Kind::ANON_FN: return "ANON_FN";
    //   case Kind::TUPLE_MEMBER: return "TUPLE_MEMBER";
    //   # case Kind::TPARAM_DEFAULT: return "TPARAM_DEFAULT";

      case Kind::CALL: return "CALL";
      case Kind::VAR_REF: return "VAR_REF";
      case Kind::TYPE_REF: return "TYPE_REF";
      case Kind::TYPE_REF_OVERLOAD: return "TYPE_REF_OVERLOAD";
      case Kind::FUNCTION_REF: return "FUNCTION_REF";
      case Kind::FUNCTION_REF_OVERLOAD: return "FUNCTION_REF_OVERLOAD";
      // case Kind::DEFN_REF: return "DEFN_REF";
    //   case Kind::PRIVATE_NAME: return "PRIVATE_NAME";
    //   case Kind::FLUENT_MEMBER: return "FLUENT_MEMBER";
    //   case Kind::TEMP_VAR: return "TEMP_VAR";
    //   case Kind::EXPLICIT_SPECIALIZE: return "EXPLICIT_SPECIALIZE";

      case Kind::BLOCK: return "BLOCK";
      case Kind::IF: return "IF";
      case Kind::WHILE: return "WHILE";
      case Kind::LOOP: return "LOOP";
      // case Kind::FOR: return "FOR";
      case Kind::FOR_IN: return "FOR_IN";
      case Kind::THROW: return "THROW";
    //   case Kind::TRY: return "TRY";
      case Kind::RETURN: return "RETURN";
    //   # case Kind::YIELD: return "YIELD";
      case Kind::BREAK: return "BREAK";
      case Kind::CONTINUE: return "CONTINUE";
      case Kind::LOCAL_VAR: return "LOCAL_VAR";
      case Kind::SWITCH: return "SWITCH"; case Kind::SWITCH_CASE: return "SWITCH_CASE";
      case Kind::MATCH: return "MATCH"; case Kind::MATCH_PATTERN: return "MATCH_PATTERN";
      case Kind::UNREACHABLE: return "UNREACHABLE";

    //   case Kind::KEYWORD_ARG: return "KEYWORD_ARG";
    //   case Kind::CONST_OBJ: return "CONST_OBJ";
    //   case Kind::CONST_ARRAY: return "CONST_ARRAY";
    //   case Kind::MODIFIED: return "MODIFIED";
    //   case Kind::CALL_STATIC: return "CALL_STATIC";
    //   case Kind::CALL_INDIRECT: return "CALL_INDIRECT";
    //   case Kind::CALL_REQUIRED: return "CALL_REQUIRED";
    //   case Kind::CALL_CTOR: return "CALL_CTOR";
    //   case Kind::CALL_EXPR: return "CALL_EXPR";
    //   case Kind::CALL_INTRINSIC: return "CALL_INTRINSIC";
    //   case Kind::FNREF_STATIC: return "FNREF_STATIC";
    //   case Kind::FNREF_INDIRECT: return "FNREF_INDIRECT";
    //   case Kind::FNREF_REQUIRED: return "FNREF_REQUIRED";
    //   case Kind::GC_ALLOC: return "GC_ALLOC";
    //   case Kind::TYPEDESC: return "TYPEDESC";
    //   case Kind::PTR_DEREF: return "PTR_DEREF";
    //   case Kind::ADDRESS_OF: return "ADDRESS_OF";

    //
    //   case Kind::BASE: return "BASE";
    //   case Kind::OPER: return "OPER";
    //   case Kind::UNARY_OP: return "UNARY_OP";
    //   case Kind::BINARY_OP: return "BINARY_OP";
    //   case Kind::CAST_OP: return "CAST_OP";

    }
    return "<Invalid AST Kind>";
  }
}
