#ifndef TEMPEST_AST_OPER_HPP
#define TEMPEST_AST_OPER_HPP 1

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

namespace tempest::sema::graph {
  class Type;
}

namespace tempest::ast {
  using llvm::StringRef;

  /** Unary operator. */
  class UnaryOp : public Node {
  public:
    const Node* arg;

    UnaryOp(Node::Kind kind, const Location& location, const Node* arg)
      : Node(kind, location)
      , arg(arg)
    {}
  };

  /** N-ary operator. */
  class Oper : public Node {
  public:
    Node* op;
    NodeList operands;

    Oper(Kind kind, const Location& location, NodeList operands)
      : Node(kind, location)
      , op(nullptr)
      , operands(operands)
    {}
  };

  /** Block statement. */
  class Block : public Node {
  public:
    NodeList stmts;
    Node* result;

    Block(const Location& location, NodeList stmts, Node* result)
      : Node(Kind::BLOCK, location)
      , stmts(stmts)
      , result(result)
    {}
  };

  /** An operator that has a test expression and multiple branches. */
  class Control : public Node {
  public:
    const Node* test;
    NodeList outcomes;

    Control(Kind kind, const Location& location, const Node* test, NodeList outcomes)
      : Node(kind, location)
      , test(test)
      , outcomes(outcomes)
    {}
  };
}

#endif
