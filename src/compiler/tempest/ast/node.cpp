#include "tempest/ast/node.hpp"

namespace tempest::ast {

  /** Singleton error node. */
  Node Node::ERROR(Node::Kind::ERROR, source::Location());

  /** Sentinel node to indicate lack of a node. */
  Node Node::ABSENT(Node::Kind::ABSENT, source::Location());

  const char* Node::KindName(Kind kind) {
    switch (kind) {
      case Kind::ERROR: return "ERROR";
      case Kind::ABSENT: return "ABSENT";
      case Kind::NULL_LITERAL: return "NULL_LITERAL";
      case Kind::SELF: return "SELF";
      case Kind::SUPER: return "SUPER";
      case Kind::IDENT: return "IDENT";
      case Kind::MEMBER: return "MEMBER";
      case Kind::SELF_NAME_REF: return "SELF_NAME_REF";
      case Kind::BUILTIN_ATTRIBUTE: return "BUILTIN_ATTRIBUTE";
      case Kind::BUILTIN_TYPE: return "BUILTIN_TYPE";
      case Kind::KEYWORD_ARG: return "KEYWORD_ARG";
      case Kind::BOOLEAN_TRUE: return "BOOLEAN_TRUE";
      case Kind::BOOLEAN_FALSE: return "BOOLEAN_FALSE";
      case Kind::CHAR_LITERAL: return "CHAR_LITERAL";
      case Kind::INTEGER_LITERAL: return "INTEGER_LITERAL";
      case Kind::FLOAT_LITERAL: return "FLOAT_LITERAL";
      case Kind::STRING_LITERAL: return "STRING_LITERAL";
      case Kind::NEGATE: return "NEGATE";
      case Kind::COMPLEMENT: return "COMPLEMENT";
      case Kind::LOGICAL_NOT: return "LOGICAL_NOT";
      case Kind::PRE_INC: return "PRE_INC";
      case Kind::POST_INC: return "POST_INC";
      case Kind::PRE_DEC: return "PRE_DEC";
      case Kind::POST_DEC: return "POST_DEC";
      case Kind::STATIC: return "STATIC";
      case Kind::CONST: return "CONST";
      case Kind::PROVISIONAL_CONST: return "PROVISIONAL_CONST";
      case Kind::OPTIONAL: return "OPTIONAL";
      case Kind::ADD: return "ADD";
      case Kind::SUB: return "SUB";
      case Kind::MUL: return "MUL";
      case Kind::DIV: return "DIV";
      case Kind::MOD: return "MOD";
      case Kind::BIT_AND: return "BIT_AND";
      case Kind::BIT_OR: return "BIT_OR";
      case Kind::BIT_XOR: return "BIT_XOR";
      case Kind::RSHIFT: return "RSHIFT";
      case Kind::LSHIFT: return "LSHIFT";
      case Kind::EQUAL: return "EQUAL";
      case Kind::REF_EQUAL: return "REF_EQUAL";
      case Kind::NOT_EQUAL: return "NOT_EQUAL";
      case Kind::LESS_THAN: return "LESS_THAN";
      case Kind::GREATER_THAN: return "GREATER_THAN";
      case Kind::LESS_THAN_OR_EQUAL: return "LESS_THAN_OR_EQUAL";
      case Kind::GREATER_THAN_OR_EQUAL: return "GREATER_THAN_OR_EQUAL";
      case Kind::IS_SUB_TYPE: return "IS_SUB_TYPE";
      case Kind::IS_SUPER_TYPE: return "IS_SUPER_TYPE";
      case Kind::ASSIGN: return "ASSIGN";
      case Kind::ASSIGN_ADD: return "ASSIGN_ADD";
      case Kind::ASSIGN_SUB: return "ASSIGN_SUB";
      case Kind::ASSIGN_MUL: return "ASSIGN_MUL";
      case Kind::ASSIGN_DIV: return "ASSIGN_DIV";
      case Kind::ASSIGN_MOD: return "ASSIGN_MOD";
      case Kind::ASSIGN_BIT_AND: return "ASSIGN_BIT_AND";
      case Kind::ASSIGN_BIT_OR: return "ASSIGN_BIT_OR";
      case Kind::ASSIGN_BIT_XOR: return "ASSIGN_BIT_XOR";
      case Kind::ASSIGN_RSHIFT: return "ASSIGN_RSHIFT";
      case Kind::ASSIGN_LSHIFT: return "ASSIGN_LSHIFT";
      case Kind::LOGICAL_AND: return "LOGICAL_AND";
      case Kind::LOGICAL_OR: return "LOGICAL_OR";
      case Kind::RANGE: return "RANGE";
      case Kind::AS_TYPE: return "AS_TYPE";
      case Kind::IS: return "IS";
      case Kind::IS_NOT: return "IS_NOT";
      case Kind::IN: return "IN";
      case Kind::NOT_IN: return "NOT_IN";
      case Kind::RETURNS: return "RETURNS";
      case Kind::LAMBDA: return "LAMBDA";
      case Kind::EXPR_TYPE: return "EXPR_TYPE";
      case Kind::RETURN: return "RETURN";
      case Kind::THROW: return "THROW";
      case Kind::TUPLE: return "TUPLE";
      case Kind::UNION: return "UNION";
      case Kind::SPECIALIZE: return "SPECIALIZE";
      case Kind::CALL: return "CALL";
      case Kind::FLUENT_MEMBER: return "FLUENT_MEMBER";
      case Kind::ARRAY_LITERAL: return "ARRAY_LITERAL";
      case Kind::LIST_LITERAL: return "LIST_LITERAL";
      case Kind::SET_LITERAL: return "SET_LITERAL";
      case Kind::CALL_REQUIRED: return "CALL_REQUIRED";
      case Kind::CALL_REQUIRED_STATIC: return "CALL_REQUIRED_STATIC";
      case Kind::LIST: return "LIST";
      case Kind::BLOCK: return "BLOCK";
      case Kind::VAR_DEFN: return "VAR_DEFN";
      case Kind::ELSE: return "ELSE";
      case Kind::FINALLY: return "FINALLY";
      case Kind::IF: return "IF";
      case Kind::WHILE: return "WHILE";
      case Kind::LOOP: return "LOOP";
      case Kind::FOR: return "FOR";
      case Kind::FOR_IN: return "FOR_IN";
      case Kind::TRY: return "TRY";
      case Kind::CATCH: return "CATCH";
      case Kind::SWITCH: return "SWITCH";
      case Kind::CASE: return "CASE";
      case Kind::MATCH: return "MATCH";
      case Kind::PATTERN: return "PATTERN";
      case Kind::MODIFIED: return "MODIFIED";
      case Kind::FUNCTION_TYPE: return "FUNCTION_TYPE";
      case Kind::BREAK: return "BREAK";
      case Kind::CONTINUE: return "CONTINUE";
      case Kind::VISIBILITY: return "VISIBILITY";
      case Kind::DEFN: return "DEFN";
      case Kind::TYPE_DEFN: return "TYPE_DEFN";
      case Kind::CLASS_DEFN: return "CLASS_DEFN";
      case Kind::STRUCT_DEFN: return "STRUCT_DEFN";
      case Kind::INTERFACE_DEFN: return "INTERFACE_DEFN";
      case Kind::EXTEND_DEFN: return "EXTEND_DEFN";
      case Kind::OBJECT_DEFN: return "OBJECT_DEFN";
      case Kind::ENUM_DEFN: return "ENUM_DEFN";
      case Kind::VAR: return "VAR";
      case Kind::LET: return "LET";
      case Kind::VAR_LIST: return "VAR_LIST";
      case Kind::ENUM_VALUE: return "ENUM_VALUE";
      case Kind::PARAMETER: return "PARAMETER";
      case Kind::TYPE_PARAMETER: return "TYPE_PARAMETER";
      case Kind::FUNCTION: return "FUNCTION";
      case Kind::DEFN_END: return "DEFN_END";
      case Kind::MODULE: return "MODULE";
      case Kind::IMPORT: return "IMPORT";
      case Kind::EXPORT: return "EXPORT";
    }
    return "<Invalid AST Kind>";
  }
}
