#ifndef TEMPEST_AST_IDENT_HPP
#define TEMPEST_AST_IDENT_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

namespace tempest::ast {
  using llvm::StringRef;

  /** Node type representing an identifier. */
  class Ident : public Node {
  public:
    const StringRef name;

    /** Construct an Ident node. */
    Ident(const Location& location, StringRef name)
      : Node(Kind::IDENT, location)
      , name(name)
    {}
  };

  /** Node type representing a member reference. */
  class MemberRef : public Node {
  public:
    const StringRef name;
    const Node* base;

    /** Construct a Member node. */
    MemberRef(const Location& location, StringRef name, Node* base)
      : Node(Kind::MEMBER, location)
      , name(name)
      , base(base)
    {}
  };

  /** Node type representing a keyword argument. */
  class KeywordArg : public Node {
  public:
    const StringRef name;
    const Node* arg;

    /** Construct a Member node. */
    KeywordArg(const Location& location, StringRef name, Node* arg)
      : Node(Kind::KEYWORD_ARG, location)
      , arg(arg)
    {}
  };

  /** Node type representing a built-in type. */
  class BuiltinType : public Node {
  public:
    enum Type {
      VOID,
      BOOL,
      CHAR,
      I8,
      I16,
      I32,
      I64,
      INT,
      U8,
      U16,
      U32,
      U64,
      UINT,
      F32,
      F64,
    };

    Type type;

    /** Construct an Ident node. */
    BuiltinType(const Location& location, Type type)
      : Node(Kind::BUILTIN_TYPE, location)
      , type(type)
    {}
  };

  /** Node type representing a built-in attribute. */
  class BuiltinAttribute : public Node {
  public:
    enum Attribute {
      INTRINSIC = 0,
      TRACEMETHOD
    };

    const Attribute attribute;

    /** Construct an Ident node. */
    BuiltinAttribute(Location& location, Attribute attribute)
      : Node(Kind::BUILTIN_ATTRIBUTE, location)
      , attribute(attribute)
    {}
  };
}

#endif
