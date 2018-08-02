#ifndef TEMPEST_AST_NODE_HPP
#define TEMPEST_AST_NODE_HPP 1

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#if TEMPEST_HAVE_OSTREAM
  #include <ostream>
#endif

namespace tempest::ast {
  using tempest::source::Location;
  using tempest::source::Locatable;

  class Node : public Locatable {
  public:
    enum class Kind {
      /* Sentinel values */
      ERROR,
      ABSENT,

      /* Built-in Values */
      NULL_LITERAL,
      SELF,
      SUPER,

      /* Identifiers */
      IDENT,
      MEMBER,
      SELF_NAME_REF,
      BUILTIN_ATTRIBUTE,
      BUILTIN_TYPE,
      KEYWORD_ARG,

      /* Literals */
      BOOLEAN_TRUE,
      BOOLEAN_FALSE,
      CHAR_LITERAL,
      INTEGER_LITERAL,
      FLOAT_LITERAL,
      STRING_LITERAL,

      /* Unary operators */
      NEGATE,
      COMPLEMENT,
      LOGICAL_NOT,
      PRE_INC,
      POST_INC,
      PRE_DEC,
      POST_DEC,
      STATIC,
      CONST,
      PROVISIONAL_CONST,
      OPTIONAL,

      /* Binary operators */
      ADD,
      SUB,
      MUL,
      DIV,
      MOD,
      BIT_AND,
      BIT_OR,
      BIT_XOR,
      RSHIFT,
      LSHIFT,
      EQUAL,
      REF_EQUAL,
      NOT_EQUAL,
      LESS_THAN,
      GREATER_THAN,
      LESS_THAN_OR_EQUAL,
      GREATER_THAN_OR_EQUAL,
      IS_SUB_TYPE,
      IS_SUPER_TYPE,
      ASSIGN,
      ASSIGN_ADD,
      ASSIGN_SUB,
      ASSIGN_MUL,
      ASSIGN_DIV,
      ASSIGN_MOD,
      ASSIGN_BIT_AND,
      ASSIGN_BIT_OR,
      ASSIGN_BIT_XOR,
      ASSIGN_RSHIFT,
      ASSIGN_LSHIFT,
      LOGICAL_AND,
      LOGICAL_OR,
      RANGE,
      AS_TYPE,
      IS,
      IS_NOT,
      IN,
      NOT_IN,
      RETURNS,
      LAMBDA,
      EXPR_TYPE,
      RETURN,
      THROW,

      /* N-ary operators */
      TUPLE_TYPE,
      UNION_TYPE,
      ARRAY_TYPE,
      SPECIALIZE,
      CALL,
      FLUENT_MEMBER,
      ARRAY_LITERAL,
      LIST_LITERAL,
      SET_LITERAL,
      CALL_REQUIRED,
      CALL_REQUIRED_STATIC,
      LIST,       // List of opions for switch cases, catch blocks, etc.

      /* Misc statements */
      BLOCK,      // A statement block
      LOCAL_LET,  // A single variable definition (ident, type, init)
      LOCAL_CONST,// A single variable definition (ident, type, init)
      ELSE,       // default for match or switch
      FINALLY,    // finally block for try

      IF,         // if-statement (test, thenBlock, elseBlock)
      WHILE,      // while-statement (test, body)
      LOOP,       // loop (body)
      FOR,        // for (vars, init, test, step, body)
      FOR_IN,     // for in (vars, iter, body)
      TRY,        // try (test, body, cases...)
      CATCH,      // catch (except-list, body)
      SWITCH,     // switch (test, cases...)
      CASE,       // switch case (values | [values...]), body
      MATCH,      // match (test, cases...)
      PATTERN,    // match pattern (pattern, body)

      /* Type operators */
      MODIFIED,
      FUNCTION_TYPE,

      /* Other statements */
      BREAK,
      CONTINUE,

      /* Definitions */
      /* TODO: Move this outside */
      VISIBILITY,

      DEFN,
      TYPE_DEFN,
      CLASS_DEFN,
      STRUCT_DEFN,
      INTERFACE_DEFN,
      TRAIT_DEFN,
      EXTEND_DEFN,
      OBJECT_DEFN,
      ENUM_DEFN,
      MEMBER_VAR,
      MEMBER_CONST,
      // VAR_LIST,   // A list of variable definitions
      ENUM_VALUE,
      PARAMETER,
      TYPE_PARAMETER,
      FUNCTION,
      DEFN_END,

      MODULE,
      IMPORT,
      EXPORT,
    };

    const Kind kind;
    const source::Location location;

    /** Construct an AST node. */
    Node(Kind kind, const Location& location) : kind(kind), location(location) {}
    Node(const Node&) = delete;

    const Location& getLocation() const { return location; }

    static inline bool isError(const Node* node) {
      return node == nullptr || node->kind == Kind::ERROR;
    }

    static inline bool isPresent(const Node* node) {
      return node != nullptr && node->kind != Kind::ABSENT;
    }

    static Node ERROR; // Sentinel node that indicate errors.
    static Node ABSENT; // Sentinel node that indicate lack of a node.

    // Return the name of the specified kind.
    static const char* KindName(Kind kind);
  };

  /** Typedef for a list of nodes. */
  typedef llvm::ArrayRef<const Node*> NodeList;

  // How to print a node kind.
  inline ::std::ostream& operator<<(::std::ostream& os, Node::Kind kind) {
    return os << Node::KindName(kind);
  }

  /** Function to pretty-print an AST graph. */
  void format(::std::ostream& out, const Node* node, bool pretty = false);
}

#endif
